/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core/common/tools/logger.hpp>

#include "imagemanager.hpp"

namespace aos::common::imagemanager {

namespace {

/***********************************************************************************************************************
 * Static
 **********************************************************************************************************************/

Error ParseDigest(const String& digest, String& algorithm, String& hash)
{
    auto [pos, err] = digest.FindSubstr(0, ":");
    if (!err.IsNone() || pos == 0 || pos + 1 >= digest.Size()) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "malformed digest"));
    }

    if (err = algorithm.Assign(String(digest.CStr(), pos)); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "malformed digest"));
    }

    if (err = hash.Assign(String(digest.CStr() + pos + 1, digest.Size() - pos - 1)); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "malformed digest"));
    }

    const auto isLowerAlnum = [](char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'); };

    if (!isLowerAlnum(algorithm[0])) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "malformed digest"));
    }

    for (const auto ch : algorithm) {
        if (!(isLowerAlnum(ch) || ch == '+' || ch == '.' || ch == '_' || ch == '-')) {
            return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "malformed digest"));
        }
    }

    for (const auto ch : hash) {
        if (!isLowerAlnum(ch)) {
            return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "malformed digest"));
        }
    }

    return ErrorEnum::eNone;
}

RetWithError<crypto::Hash> GetSupportedHash(const String& hashName)
{
    StaticString<crypto::cAlgLen> upperHash = hashName;

    (void)upperHash.ToUpper();

    if (upperHash == "SHA256") {
        return {crypto::HashEnum::eSHA256, ErrorEnum::eNone};
    } else if (upperHash == "SHA512") {
        return {crypto::HashEnum::eSHA512, ErrorEnum::eNone};
    } else {
        return {{}, AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "unsupported hashing algorithm"))};
    }
}

} // namespace

/***********************************************************************************************************************
 * Public
 **********************************************************************************************************************/

Error ImageManager::Start()
{
    LOG_DBG() << "Start image manager";

    return ErrorEnum::eNotSupported;
}

Error ImageManager::Stop()
{
    LOG_DBG() << "Stop image manager";

    return ErrorEnum::eNotSupported;
}

Error ImageManager::GetItemsStatuses(Array<UpdateItemStatus>& statuses)
{
    LOG_DBG() << "Get items statuses";

    LockGuard lock {mMutex};

    auto items = MakeUnique<StaticArray<DeployableItemInfo, cMaxNumStoredItems>>(mAllocator);
    if (!items) {
        return AOS_ERROR_WRAP(ErrorEnum::eNoMemory);
    }

    if (auto err = mStorage->GetAllDeployableItems(*items); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    for (const auto& item : *items) {
        auto status = CreateItemStatus(item.mItemID, item.mType, item.mVersion);

        if (const auto* itemInProgress = FindInProgressItem(item.mItemID, item.mVersion); itemInProgress != nullptr) {
            status = *itemInProgress;
        } else {
            status.mState = item.mState;
            status.mError = item.mError;
        }

        if (auto err = statuses.PushBack(status); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }
    }

    return ErrorEnum::eNone;
}

Error ImageManager::SubscribeListener(ItemStatusListenerItf& listener)
{
    LockGuard lock {mListenersMutex};

    LOG_DBG() << "Subscribe listener";

    if (mListeners.Contains(&listener)) {
        return AOS_ERROR_WRAP(ErrorEnum::eAlreadyExist);
    }

    if (auto err = mListeners.PushBack(&listener); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error ImageManager::UnsubscribeListener(ItemStatusListenerItf& listener)
{
    LockGuard lock {mListenersMutex};

    LOG_DBG() << "Unsubscribe listener";

    if (mListeners.Remove(&listener) == 0) {
        return AOS_ERROR_WRAP(ErrorEnum::eNotFound);
    }

    return ErrorEnum::eNone;
}

Error ImageManager::GetBlobPath(const String& digest, String& path) const
{
    LOG_DBG() << "Get blob path" << Log::Field("digest", digest);

    StaticString<oci::cDigestLen> algorithm;
    StaticString<oci::cDigestLen> hash;

    if (auto err = ParseDigest(digest, algorithm, hash); !err.IsNone()) {
        return err;
    }

    if (auto err = path.Assign(fs::JoinPath(mConfig.mImagePath, cBlobsFolder, algorithm, hash)); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Protected
 **********************************************************************************************************************/

Error ImageManager::Init(const Config& config, DownloadBlobInfoProviderItf& blobInfoProvider, StorageItf& storage,
    spaceallocator::SpaceAllocatorItf& spaceAllocator, fs::FileInfoProviderItf& fileInfoProvider,
    oci::OCISpecItf& ociSpec, fs::FSPlatformItf& fsPlatform, AllocatorItf& allocator)
{
    LOG_DBG() << "Init image manager";

    mConfig           = config;
    mBlobInfoProvider = &blobInfoProvider;
    mStorage          = &storage;
    mSpaceAllocator   = &spaceAllocator;
    mFileInfoProvider = &fileInfoProvider;
    mOCISpec          = &ociSpec;
    mFSPlatform       = &fsPlatform;
    mAllocator        = &allocator;

    LOG_DBG() << "Config" << Log::Field("imagePath", mConfig.mImagePath) << Log::Field("partLimit", mConfig.mPartLimit)
              << Log::Field("itemCacheTTL", mConfig.mItemCacheTTL)
              << Log::Field("removeOutdatedPeriod", mConfig.mRemoveOutdatedPeriod);

    if (auto err = fs::MakeDirAll(fs::JoinPath(mConfig.mImagePath, cBlobsFolder)); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error ImageManager::InstallItem(const InstallItemInfo& itemInfo)
{
    LOG_INF() << "Install item" << Log::Field("itemID", itemInfo.mItemID) << Log::Field("version", itemInfo.mVersion)
              << Log::Field("type", itemInfo.mType) << Log::Field("rootDigest", itemInfo.mRootDigest);

    if (auto err = LockItem(itemInfo.mItemID, itemInfo.mVersion); !err.IsNone()) {
        return err;
    }

    auto unlockItem = DeferRelease([this, &itemInfo]() { UnlockItem(itemInfo.mItemID, itemInfo.mVersion); });

    auto itemInProgress = FindInProgressItem(itemInfo.mItemID, itemInfo.mVersion);
    if (itemInProgress == nullptr) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "item is not in progress"));
    }

    *itemInProgress = CreateItemStatus(itemInfo.mItemID, itemInfo.mType, itemInfo.mVersion, ItemStateEnum::eUnknown);

    Error              err;
    DeployableItemInfo storedInfo;

    err = StartItemInstall(itemInfo, storedInfo);
    if (!err.IsNone()) {
        return err;
    }

    auto finishInstall = DeferRelease([this, &storedInfo, &err]() {
        if (!err.IsNone()) {
            if (auto e = SetStoredItemState(storedInfo, ItemStateEnum::eFailed, err); !e.IsNone()) {
                LOG_ERR() << "Failed to set stored item state" << Log::Field(e);
            }
        }
    });

    if (err = InstallItemBlobs(itemInfo); !err.IsNone()) {
        return err;
    }

    if (err = mFSPlatform->Sync(mConfig.mImagePath); !err.IsNone()) {
        return err;
    }

    if (err = SetStoredItemState(storedInfo, ItemStateEnum::eInstalled); !err.IsNone()) {
        return err;
    }

    return err;
}

Error ImageManager::RemoveItem(const String& itemID, const String& version)
{
    LOG_INF() << "Remove item" << Log::Field("itemID", itemID) << Log::Field("version", version);

    if (auto err = LockItem(itemID, version); !err.IsNone()) {
        return err;
    }

    auto unlockItem = DeferRelease([this, &itemID, &version]() { UnlockItem(itemID, version); });

    DeployableItemInfo item;
    ItemStateEnum      prevState {};

    if (auto err = mStorage->GetDeployableItem(itemID, version, item); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (item.mState != ItemStateEnum::eRemoved) {
        prevState = item.mState == ItemStateEnum::eInstalling ? ItemStateEnum::eFailed : item.mState.GetValue();

        if (auto* itemInProgress = FindInProgressItem(itemID, version); itemInProgress != nullptr) {
            itemInProgress->mType  = item.mType;
            itemInProgress->mState = ItemStateEnum::eRemoving;
        }
    }

    if (item.mState == ItemStateEnum::eRemoved) {
        LOG_DBG() << "Item already removed, add it as outdated" << Log::Field("itemID", itemID)
                  << Log::Field("version", version);

        unlockItem.Reset();

        if (auto err = mSpaceAllocator->AddOutdatedItem(itemID, version, item.mTimestamp); !err.IsNone()) {
            LOG_ERR() << "Can't add outdated item" << Log::Field("itemID", itemID) << Log::Field("version", version)
                      << Log::Field(err);

            return AOS_ERROR_WRAP(err);
        }

        return ErrorEnum::eNone;
    }

    NotifyItemStatusChanged(CreateItemStatus(itemID, item.mType, version, ItemStateEnum::eRemoving));

    Error err;

    LockGuard lock {mMutex};

    item.mState     = ItemStateEnum::eRemoved;
    item.mTimestamp = Time::Now();
    item.mError     = ErrorEnum::eNone;

    if (err = mStorage->UpdateDeployableItem(item); !err.IsNone()) {
        err = AOS_ERROR_WRAP(err);
    }

    if (!err.IsNone()) {
        LOG_ERR() << "Can't remove item" << Log::Field("itemID", itemID) << Log::Field("version", version)
                  << Log::Field(err);

        NotifyItemStatusChanged(CreateItemStatus(itemID, item.mType, version, prevState, err));

        return err;
    }

    NotifyItemStatusChanged(CreateItemStatus(itemID, item.mType, version, ItemStateEnum::eRemoved));

    unlockItem.Reset();

    if (err = mSpaceAllocator->AddOutdatedItem(itemID, version, item.mTimestamp); !err.IsNone()) {
        LOG_ERR() << "Can't add outdated item" << Log::Field("itemID", itemID) << Log::Field("version", version)
                  << Log::Field(err);

        return AOS_ERROR_WRAP(err);
    }

    LOG_INF() << "Item removed" << Log::Field("itemID", itemID) << Log::Field("version", version);

    return ErrorEnum::eNone;
}

/***********************************************************************************************************************
 * Private
 **********************************************************************************************************************/

void ImageManager::NotifyItemStatusChanged(const UpdateItemStatus& status)
{
    LockGuard lock {mListenersMutex};

    LOG_DBG() << "Item status changed" << Log::Field("itemID", status.mItemID) << Log::Field("type", status.mType)
              << Log::Field("version", status.mVersion) << Log::Field("state", status.mState)
              << Log::Field(status.mError);

    for (auto* listener : mListeners) {
        listener->OnItemStatusChanged(status);
    }
}

void ImageManager::NotifyItemRemovedFromCache(const String& itemID, const String& version)
{
    LockGuard lock {mListenersMutex};

    LOG_DBG() << "Item removed from cache" << Log::Field("itemID", itemID) << Log::Field("version", version);

    for (auto* listener : mListeners) {
        listener->OnItemRemovedFromCache(itemID, version);
    }
}

Error ImageManager::ValidateLayer(
    const String& path, const oci::ContentDescriptor& descriptor, const String& /* diffID */)
{
    return ValidateBlob(path, descriptor.mDigest, descriptor.mSize);
}

Error ImageManager::ValidateBlob(const String& path, const String& digest, size_t size)
{
    LOG_DBG() << "Validate blob" << Log::Field("digest", digest) << Log::Field("path", path);

    auto [exists, err] = fs::FileExist(path);
    if (!exists) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "file not found"));
    }
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    StaticString<oci::cDigestLen> algStr;
    StaticString<oci::cDigestLen> hash;

    if (err = ParseDigest(digest, algStr, hash); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    crypto::Hash alg;

    Tie(alg, err) = GetSupportedHash(algStr);
    if (!err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }
    auto hashBytes = MakeUnique<StaticArray<uint8_t, cBlobHashSize>>(mAllocator);

    if (err = hash.HexToByteArray(*hashBytes); !err.IsNone()) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "malformed digest hash"));
    }

    fs::FileInfo fileInfo;

    if (err = mFileInfoProvider->GetFileInfo(path, fileInfo, crypto::HashEnum::eSHA256); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (size && fileInfo.mSize != size) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "size mismatch"));
    }

    if (fileInfo.mCheckSum != *hashBytes) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "checksum mismatch"));
    }

    return ErrorEnum::eNone;
}

Error ImageManager::GetBlobInfo(const String& digest, BlobInfo& info)
{
    while (true) {
        if (auto err = mBlobInfoProvider->GetBlobInfo(digest, info); !err.IsNone()) {
            UniqueLock<Mutex> lock(mMutex);

            LOG_ERR() << "Failed to get blob info" << Log::Field("digest", digest) << Log::Field(err);

            if (err = mCondVar.Wait(lock, cRetryTimeout); !err.IsNone() && err != ErrorEnum::eTimeout) {
                return AOS_ERROR_WRAP(err);
            }

            LOG_DBG() << "Retrying get blob info" << Log::Field("digest", digest);

            continue;
        }

        break;
    }

    return ErrorEnum::eNone;
}

Error ImageManager::DownloadItemBlob(const String& path, const String& digest)
{
    auto blobInfo = MakeUnique<BlobInfo>(mAllocator);
    if (!blobInfo) {
        return AOS_ERROR_WRAP(ErrorEnum::eNoMemory);
    }

    if (auto err = GetBlobInfo(digest, *blobInfo); !err.IsNone()) {
        return err;
    }

    StaticString<cFilePathLen> parentPath;

    if (auto err = fs::ParentPath(path, parentPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = fs::MakeDirAll(parentPath); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    Error err;

    auto space = mSpaceAllocator->AllocateSpace(blobInfo->mSize);

    LOG_DBG() << "Download blob" << Log::Field("digest", digest) << Log::Field("size", blobInfo->mSize);

    if (err = DownloadBlob(path, *blobInfo); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error ImageManager::InstallItemBlob(const String& path, const InstallItemInfo& itemInfo,
    const oci::ContentDescriptor& descriptor, bool isLayer, const String& diffID)
{
    LOG_DBG() << "Install blob" << Log::Field("digest", descriptor.mDigest)
              << Log::Field("mediaType", descriptor.mMediaType) << Log::Field("size", descriptor.mSize);

    if (auto err = LockBlob(descriptor.mDigest); !err.IsNone()) {
        return err;
    }

    auto unlockBlob = DeferRelease([this, digest = &descriptor.mDigest]() { UnlockBlob(*digest); });

    Error err;

    if (isLayer) {
        err = ValidateLayer(path, descriptor, diffID);
    } else {
        err = ValidateBlob(path, descriptor.mDigest, descriptor.mSize);
    }

    if (err.IsNone()) {
        return ErrorEnum::eNone;
    }

    if (!err.Is(ErrorEnum::eNotFound)) {
        LOG_ERR() << "Blob validation failed" << Log::Field("digest", descriptor.mDigest) << Log::Field("error", err);
    }

    if (err = RemoveItemBlob(path, descriptor, isLayer, diffID); !err.IsNone()) {
        return err;
    }

    auto itemInProgress = FindInProgressItem(itemInfo.mItemID, itemInfo.mVersion);
    if (!itemInProgress) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "item in progress not found"));
    }

    if (err = SetInProgressItemState(itemInfo, ItemStateEnum::eDownloading); !err.IsNone()) {
        LOG_ERR() << "Failed to set in-progress item state" << Log::Field(err);
    }

    if (err = DownloadItemBlob(path, descriptor.mDigest); !err.IsNone()) {
        return err;
    }

    if (err = ValidateBlob(path, descriptor.mDigest, descriptor.mSize); !err.IsNone()) {
        return err;
    }

    if (isLayer) {
        if (err = UnpackLayer(path, descriptor, diffID); !err.IsNone()) {
            return err;
        }
    }

    if (err = AddItemBlob(itemInfo.mItemID, itemInfo.mVersion, descriptor, isLayer, diffID); !err.IsNone()) {
        return err;
    }

    return ErrorEnum::eNone;
}

Error ImageManager::RemoveItemBlob(
    const String& path, const oci::ContentDescriptor& descriptor, bool isLayer, const String& diffID)
{
    Error  err;
    size_t layerSize = 0;
    size_t blobSize  = 0;

    if (isLayer) {
        Tie(layerSize, err) = RemoveLayer(path, descriptor, diffID);
        if (!err.IsNone()) {
            return err;
        }
    }

    Tie(blobSize, err) = RemoveBlob(path, descriptor.mDigest);
    if (!err.IsNone()) {
        return err;
    }

    if (layerSize + blobSize > 0) {
        mSpaceAllocator->FreeSpace(layerSize + blobSize);
    }

    return ErrorEnum::eNone;
}

RetWithError<size_t> ImageManager::RemoveBlob(const String& path, const String& digest)
{
    LOG_DBG() << "Remove blob" << Log::Field("digest", digest) << Log::Field("path", path);

    auto [exists, err] = fs::FileExist(path);
    if (!err.IsNone()) {
        return {0, AOS_ERROR_WRAP(err)};
    }

    if (!exists) {
        return {0, ErrorEnum::eNone};
    }

    size_t size = 0;

    if (Tie(size, err) = fs::CalculateSize(*mAllocator, path); !err.IsNone()) {
        return {0, AOS_ERROR_WRAP(err)};
    }

    if (err = fs::RemoveAll(path); !err.IsNone()) {
        return {0, AOS_ERROR_WRAP(err)};
    }

    return {size, ErrorEnum::eNone};
}

RetWithError<size_t> ImageManager::RemoveOutdatedItem(const String& itemID, const String& version)
{
    LOG_DBG() << "Remove outdated item" << Log::Field("itemID", itemID) << Log::Field("version", version);

    return {0, ErrorEnum::eNotSupported};
}

UpdateItemStatus ImageManager::CreateItemStatus(
    const String& itemID, UpdateItemType type, const String& version, ItemStateEnum state, const Error& error)
{
    UpdateItemStatus status;

    status.mItemID  = itemID;
    status.mType    = type;
    status.mVersion = version;
    status.mState   = state;
    status.mError   = error;

    return status;
}

Error ImageManager::LockItem(const String& itemID, const String& version)
{
    UniqueLock lock {mMutex};

    if (auto err = mCondVar.Wait(
            lock, [&]() { return FindInProgressItem(itemID, version) == nullptr && !mItemsInProgress.IsFull(); });
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    UpdateItemStatus item;

    item.mItemID  = itemID;
    item.mVersion = version;

    if (auto err = mItemsInProgress.PushBack(item); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

void ImageManager::UnlockItem(const String& itemID, const String& version)
{
    {
        LockGuard lock {mMutex};

        mItemsInProgress.RemoveIf(
            [&](const UpdateItemStatus& item) { return item.mItemID == itemID && item.mVersion == version; });
    }

    if (auto err = mCondVar.NotifyAll(); !err.IsNone()) {
        LOG_ERR() << "Can't notify condition variable" << Log::Field(err);
    }
}

UpdateItemStatus* ImageManager::FindInProgressItem(const String& itemID, const String& version)
{
    UniqueLock lock {mMutex};

    auto it = mItemsInProgress.FindIf(
        [&](const UpdateItemStatus& item) { return item.mItemID == itemID && item.mVersion == version; });

    return it != mItemsInProgress.end() ? it : nullptr;
}

Error ImageManager::LockBlob(const String& digest)
{
    UniqueLock lock {mMutex};

    if (auto err = mCondVar.Wait(lock, [&]() { return !IsBlobInProgress(digest) && !mBlobsInProgress.IsFull(); });
        !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto err = mBlobsInProgress.PushBack(digest); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

bool ImageManager::TryLockBlob(const String& digest)
{
    LockGuard lock {mMutex};

    if (IsBlobInProgress(digest) || mBlobsInProgress.IsFull()) {
        return false;
    }

    return mBlobsInProgress.PushBack(digest).IsNone();
}

void ImageManager::UnlockBlob(const String& digest)
{
    {
        LockGuard lock {mMutex};

        mBlobsInProgress.Remove(digest);
    }

    if (auto err = mCondVar.NotifyAll(); !err.IsNone()) {
        LOG_ERR() << "Can't notify condition variable" << Log::Field(err);
    }
}

Error ImageManager::StartItemInstall(const InstallItemInfo& itemInfo, DeployableItemInfo& storedInfo)
{
    auto err = mStorage->GetDeployableItem(itemInfo.mItemID, itemInfo.mVersion, storedInfo);
    if (!err.IsNone() && !err.Is(ErrorEnum::eNotFound)) {
        return AOS_ERROR_WRAP(err);
    }

    if (err.IsNone()) {
        if (storedInfo.mRootDigest != itemInfo.mRootDigest) {
            return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "root digest mismatch"));
        }

        if (storedInfo.mState == ItemStateEnum::eRemoved) {
            if (err = mSpaceAllocator->RestoreOutdatedItem(itemInfo.mItemID, itemInfo.mVersion); !err.IsNone()) {
                LOG_ERR() << "Failed to restore outdated item" << Log::Field(err);
            }
        }

        if (err = SetStoredItemState(storedInfo, ItemStateEnum::eInstalling, ErrorEnum::eNone); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        return ErrorEnum::eNone;
    }

    storedInfo.mItemID     = itemInfo.mItemID;
    storedInfo.mType       = itemInfo.mType;
    storedInfo.mVersion    = itemInfo.mVersion;
    storedInfo.mRootDigest = itemInfo.mRootDigest;
    storedInfo.mState      = ItemStateEnum::eInstalling;
    storedInfo.mTimestamp  = Time::Now();
    storedInfo.mError      = ErrorEnum::eNone;

    if (err = mStorage->AddDeployableItem(storedInfo); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (err = SetInProgressItemState(itemInfo, ItemStateEnum::eInstalling); !err.IsNone()) {
        LOG_ERR() << "Failed to set in-progress item state" << Log::Field(err);
    }

    return ErrorEnum::eNone;
}

Error ImageManager::InstallItemBlobs(const InstallItemInfo& itemInfo)
{
    StaticString<cFilePathLen> path;

    if (auto err = GetBlobPath(itemInfo.mRootDigest, path); !err.IsNone()) {
        return err;
    }

    if (!itemInfo.mIsRootIndex) {
        return InstallManifestBlobs(
            path, itemInfo, oci::ContentDescriptor {itemInfo.mRootDigest, oci::cMediaTypeImageManifest, 0});
    }

    if (auto err = InstallItemBlob(
            path, itemInfo, oci::ContentDescriptor {itemInfo.mRootDigest, oci::cMediaTypeImageManifest, 0}, false);
        !err.IsNone()) {
        return err;
    }

    auto index = MakeUnique<oci::ImageIndex>(mAllocator);
    if (!index) {
        return AOS_ERROR_WRAP(ErrorEnum::eNoMemory);
    }

    if (auto err = mOCISpec->LoadImageIndex(path, *index); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    for (const auto& manifest : index->mManifests) {
        if (auto err = GetBlobPath(itemInfo.mRootDigest, path); !err.IsNone()) {
            return err;
        }

        if (auto err = InstallManifestBlobs(path, itemInfo, manifest); !err.IsNone()) {
            return err;
        }
    }

    return ErrorEnum::eNone;
}

Error ImageManager::InstallManifestBlobs(
    const String& path, const InstallItemInfo& itemInfo, const oci::ContentDescriptor& descriptor)
{
    LOG_DBG() << "Install manifest" << Log::Field("digest", descriptor.mDigest);

    if (auto err = InstallItemBlob(path, itemInfo, descriptor); !err.IsNone()) {
        return err;
    }

    auto manifest = MakeUnique<oci::ImageManifest>(mAllocator);
    if (!manifest) {
        return AOS_ERROR_WRAP(ErrorEnum::eNoMemory);
    }

    if (auto err = mOCISpec->LoadImageManifest(path, *manifest); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    StaticString<cFilePathLen> blobPath;

    if (manifest->mItemConfig.HasValue()) {
        if (auto err = GetBlobPath(manifest->mItemConfig->mDigest, blobPath); !err.IsNone()) {
            return err;
        }

        if (auto err = InstallItemBlob(blobPath, itemInfo, *manifest->mItemConfig); !err.IsNone()) {
            return err;
        }
    }

    UniquePtr<oci::ImageConfig> imageConfig;

    if (!manifest->mConfig.mDigest.IsEmpty()) {
        if (auto err = GetBlobPath(manifest->mItemConfig->mDigest, blobPath); !err.IsNone()) {
            return err;
        }

        if (auto err = InstallItemBlob(blobPath, itemInfo, manifest->mConfig); !err.IsNone()) {
            return err;
        }

        imageConfig = MakeUnique<oci::ImageConfig>(mAllocator);
        if (!imageConfig) {
            return AOS_ERROR_WRAP(ErrorEnum::eNoMemory);
        }

        if (auto err = mOCISpec->LoadImageConfig(blobPath, *imageConfig); !err.IsNone()) {
            return AOS_ERROR_WRAP(err);
        }

        if (imageConfig->mRootfs.mDiffIDs.Size() < manifest->mLayers.Size()) {
            return AOS_ERROR_WRAP(Error(ErrorEnum::eInvalidArgument, "diff ID's size mismatch"));
        }
    }

    for (size_t i = 0; i < manifest->mLayers.Size(); ++i) {
        StaticString<oci::cDigestLen> diffID;

        if (imageConfig) {
            diffID = imageConfig->mRootfs.mDiffIDs[i];
        }

        if (auto err = GetBlobPath(manifest->mLayers[i].mDigest, blobPath); !err.IsNone()) {
            return err;
        }

        if (auto err = InstallItemBlob(blobPath, itemInfo, manifest->mLayers[i], true, diffID); !err.IsNone()) {
            return err;
        }
    }

    return ErrorEnum::eNone;
}

Error ImageManager::AddItemBlob(const String& itemID, const String& version, const oci::ContentDescriptor& descriptor,
    bool isLayer, const String& diffID)
{
    if (auto err = mStorage->AddItemBlob(itemID, version, ItemBlobInfo {descriptor, isLayer, diffID}); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    return ErrorEnum::eNone;
}

Error ImageManager::SetInProgressItemState(const InstallItemInfo& itemInfo, ItemStateEnum state, const Error& error)
{
    auto itemInProgress = FindInProgressItem(itemInfo.mItemID, itemInfo.mVersion);
    if (!itemInProgress) {
        return AOS_ERROR_WRAP(Error(ErrorEnum::eNotFound, "item not in progress"));
    }

    if (itemInProgress->mState != state) {
        NotifyItemStatusChanged(*itemInProgress);
    }

    itemInProgress->mState = state;
    itemInProgress->mError = error;

    return ErrorEnum::eNone;
}

Error ImageManager::SetStoredItemState(DeployableItemInfo& item, ItemStateEnum state, const Error& error)
{
    auto stateChanged = (item.mState != state);

    item.mState     = state;
    item.mTimestamp = Time::Now();
    item.mError     = error;

    if (auto err = mStorage->UpdateDeployableItem(item); !err.IsNone()) {
        return AOS_ERROR_WRAP(err);
    }

    if (auto itemInProgress = FindInProgressItem(item.mItemID, item.mVersion); itemInProgress) {
        itemInProgress->mState = state;
        itemInProgress->mError = error;
    }

    if (stateChanged) {
        NotifyItemStatusChanged(item);
    }

    return ErrorEnum::eNone;
}

} // namespace aos::common::imagemanager
