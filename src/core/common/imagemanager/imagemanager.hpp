/*
 * Copyright (C) 2026 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_CORE_COMMON_IMAGEMANAGER_IMAGEMANAGER_HPP_
#define AOS_CORE_COMMON_IMAGEMANAGER_IMAGEMANAGER_HPP_

#include <core/common/consts.hpp>
#include <core/common/ocispec/itf/ocispec.hpp>
#include <core/common/spaceallocator/itf/spaceallocator.hpp>
#include <core/common/tools/fs.hpp>
#include <core/common/tools/list.hpp>
#include <core/common/tools/memory.hpp>
#include <core/common/tools/noncopyable.hpp>
#include <core/common/tools/thread.hpp>

#include "itf/blobinfoprovider.hpp"
#include "itf/itemstatusprovider.hpp"
#include "itf/storage.hpp"

#include "config.hpp"

namespace aos::common::imagemanager {

/**
 * Common image manager.
 *
 * Implements the part of the image manager shared by CM and SM. CM and SM image managers are derived from this class
 * and specialize blob download and layer handling.
 */
class ImageManager : public ItemStatusProviderItf, public LocalBlobInfoProviderItf, private NonCopyable {
public:
    /**
     * Destructor.
     */
    virtual ~ImageManager() = default;

    /**
     * Starts image manager.
     *
     * @return Error.
     */
    virtual Error Start();

    /**
     * Stops image manager. Waits until all InstallItem and RemoveItem calls in progress return.
     *
     * @return Error.
     */
    virtual Error Stop();

    /**
     * Retrieves deployable items statuses. Reports one status per item version in the storage. An item version being
     * installed, whose eInstalling state is already notified, is reported with its last notified state (eInstalling or
     * eDownloading) until its final state is notified. An item version being removed, whose eRemoving state is already
     * notified, is reported as eRemoving until its eRemoved state is set in the storage or the storage update fails.
     * Otherwise the stored state is reported. mError is set only for eFailed items and is taken from the storage.
     * mPreinstalled is always false. ePending is never reported.
     *
     * @note The statuses are appended to the output array, which is not cleared.
     *
     * @param[out] statuses list of deployable items statuses.
     * @return Error: eNoMemory if the statuses don't fit the output array, storage errors as is.
     */
    Error GetItemsStatuses(Array<UpdateItemStatus>& statuses) override;

    /**
     * Subscribes status notifications. Can be called at any time, including from a listener callback.
     *
     * @note The final eInstalled or eFailed status of InstallItem and the eRemoving status of RemoveItem are notified
     * while the item is still in progress, so a listener must not call InstallItem or RemoveItem synchronously from the
     * callback.
     *
     * @param listener status listener.
     * @return Error: eAlreadyExist if the listener is already subscribed.
     */
    Error SubscribeListener(ItemStatusListenerItf& listener) override;

    /**
     * Unsubscribes from status notifications. Can be called at any time, including from a listener callback.
     *
     * @note A notification already in progress on another thread may still call the listener after
     * UnsubscribeListener returns, so the listener must stay alive until the image manager is stopped or until the
     * notification in progress returns.
     *
     * @param listener status listener.
     * @return Error: eNotFound if the listener is not subscribed.
     */
    Error UnsubscribeListener(ItemStatusListenerItf& listener) override;

    /**
     * Returns local blob path.
     *
     * @param digest blob digest.
     * @param[out] path local blob path.
     * @return Error.
     */
    Error GetBlobPath(const String& digest, String& path) const override;

    /**
     * Returns item remover which should be passed to the space allocator to remove outdated items.
     *
     * @return spaceallocator::ItemRemoverItf&.
     */
    spaceallocator::ItemRemoverItf& GetItemRemover() { return mItemRemover; }

protected:
    /**
     * Install item info.
     */
    struct InstallItemInfo {
        StaticString<cIDLen>          mItemID;
        UpdateItemType                mType;
        StaticString<cVersionLen>     mVersion;
        StaticString<oci::cDigestLen> mRootDigest;
        bool                          mIsRootIndex {};
    };

    /**
     * Initializes image manager. Derived classes call it from their own Init.
     *
     * @note The allocator must provide, per concurrent InstallItem (up to cMaxNumConcurrentItems), one
     * oci::ImageIndex, one oci::ImageManifest, one oci::ImageConfig and one blob hash buffer of cBlobHashSize at the
     * same time, plus one StaticArray<DeployableItemInfo, cMaxNumStoredItems> for GetItemsStatuses.
     *
     * @param config image manager config.
     * @param downloadBlobInfoProvider download blob info provider.
     * @param storage image manager storage.
     * @param spaceAllocator space allocator.
     * @param fileInfoProvider file info provider.
     * @param ociSpec OCI spec interface.
     * @param fsPlatform file system platform interface.
     * @param allocator allocator for temporary objects.
     * @return Error.
     */
    Error Init(const Config& config, DownloadBlobInfoProviderItf& downloadBlobInfoProvider, StorageItf& storage,
        spaceallocator::SpaceAllocatorItf& spaceAllocator, fs::FileInfoProviderItf& fileInfoProvider,
        oci::OCISpecItf& ociSpec, fs::FSPlatformItf& fsPlatform, AllocatorItf& allocator);

    /**
     * Installs item. Waits while the same item version is being installed or removed, or while
     * cMaxNumConcurrentItems items are being processed.
     *
     * @note The final eInstalled or eFailed status is notified while the item is still in progress, so a listener
     * must not call InstallItem or RemoveItem synchronously from the callback.
     *
     * @param itemInfo item info.
     * @return Error: eInvalidArgument if the root digest is malformed, the item version is stored with a different
     * root digest or a diff ID is missing in the image config, eNoMemory if the storage is full and there is no
     * outdated item to remove, other errors as is.
     */
    Error InstallItem(const InstallItemInfo& itemInfo);

    /**
     * Removes item. Sets the item state to eRemoved in the storage, keeping the item blobs, and adds the item as
     * outdated to the space allocator. If the item is already removed, adds it as outdated to the space allocator again
     * with its stored timestamp, without notifications and storage update, so an item whose previous registration
     * failed is registered. Waits while the same item version is being installed or removed, or while
     * cMaxNumConcurrentItems items are being processed.
     *
     * @note The eRemoving status and the previous item status on the storage update failure are notified while the
     * item is still in progress, so a listener must not call InstallItem or RemoveItem synchronously from the callback.
     *
     * @param itemID item ID.
     * @param version item version.
     * @return Error: eNotFound if the item version is not stored, other errors as is.
     */
    Error RemoveItem(const String& itemID, const String& version);

    /**
     * Removes blob. Doesn't call the space allocator: the caller frees the returned size.
     *
     * @param digest blob digest.
     * @param path blob path.
     * @return RetWithError<size_t>: freed size, 0 if the blob doesn't exist.
     */

    /**
     * Returns space allocator.
     *
     * @return spaceallocator::SpaceAllocatorItf&.
     */
    spaceallocator::SpaceAllocatorItf& GetSpaceAllocator() const { return *mSpaceAllocator; }

private:
    static constexpr auto cBlobsFolder       = "blobs";
    static constexpr auto cBlobHashSize      = Max(crypto::cSHA256Size, crypto::cSHA512Size);
    static constexpr auto cMaxNumListeners   = 4;
    static constexpr auto cMaxNumStoredItems = cMaxNumUpdateItems * 2;
    static constexpr auto cMaxNumItemBlobs   = 1 + oci::cMaxNumManifests * (3 + oci::cMaxNumLayers);
    static constexpr auto cRetryTimeout      = Time::cSeconds * 2;

    /**
     * Downloads blob. Called by InstallBlob, derived classes implement it but must not call it directly.
     *
     * @param path destination blob path.
     * @param blobInfo blob information.
     * @return Error.
     */
    virtual Error DownloadBlob(const String& path, const BlobInfo& blobInfo) = 0;

    /**
     * Cancels ongoing blob download.
     *
     * @param digest blob digest.
     * @return Error.
     */
    virtual Error CancelDownload(const String& digest) = 0;

    /**
     * Validates installed layer. Derived classes can also validate unpacked layer data.
     *
     * @param path layer blob path.
     * @param descriptor layer descriptor.
     * @param diffID diff ID, empty if item has no image config.
     * @return Error.
     */
    virtual Error ValidateLayer(const String& path, const oci::ContentDescriptor& descriptor, const String& diffID);

    /**
     * Removes installed layer. Derived classes can also remove unpacked layer data.
     *
     * @param path layer blob path.
     * @param descriptor layer descriptor.
     * @param diffID diff ID, empty if item has no image config.
     * @return RetWithError<size_t>: freed size, 0 if the layer doesn't exist.
     */
    virtual RetWithError<size_t> RemoveLayer(
        const String& path, const oci::ContentDescriptor& descriptor, const String& diffID)
    {
        return 0;
    }

    /**
     * Unpacks installed layer. Derived classes can also unpack layer data.
     *
     * @param path layer blob path.
     * @param descriptor layer descriptor.
     * @param diffID diff ID, empty if item has no image config.
     * @return Error.
     */
    virtual Error UnpackLayer(const String& path, const oci::ContentDescriptor& descriptor, const String& diffID)
    {
        return ErrorEnum::eNone;
    }

    class ItemRemover : public spaceallocator::ItemRemoverItf {
    public:
        explicit ItemRemover(ImageManager& imageManager)
            : mImageManager(imageManager)
        {
        }

        RetWithError<size_t> RemoveItem(const String& id, const String& version) override
        {
            return mImageManager.RemoveOutdatedItem(id, version);
        }

    private:
        ImageManager& mImageManager;
    };

    Error                ValidateBlob(const String& path, const String& digest, size_t size = 0);
    Error                GetBlobInfo(const String& digest, BlobInfo& info);
    Error                DownloadItemBlob(const String& path, const String& digest);
    RetWithError<size_t> RemoveBlob(const String& path, const String& digest);
    Error                RemoveItemBlob(
                       const String& path, const oci::ContentDescriptor& descriptor, bool isLayer = false, const String& diffID = "");
    Error InstallItemBlob(const String& path, const InstallItemInfo& itemInfo, const oci::ContentDescriptor& descriptor,
        bool isLayer = false, const String& diffID = "");
    Error InstallItemBlobs(const InstallItemInfo& itemInfo);
    Error InstallManifestBlobs(
        const String& path, const InstallItemInfo& itemInfo, const oci::ContentDescriptor& descriptor);
    Error StartItemInstall(const InstallItemInfo& itemInfo, DeployableItemInfo& storedInfo);
    Error AddItemBlob(const String& itemID, const String& version, const oci::ContentDescriptor& descriptor,
        bool isLayer = false, const String& diffID = "");

    UpdateItemStatus CreateItemStatus(const String& itemID, UpdateItemType type, const String& version,
        ItemStateEnum state, const Error& error = ErrorEnum::eNone);
    Error            SetInProgressItemState(
                   const InstallItemInfo& itemInfo, ItemStateEnum state, const Error& error = ErrorEnum::eNone);
    Error SetStoredItemState(DeployableItemInfo& item, ItemStateEnum state, const Error& error = ErrorEnum::eNone);

    RetWithError<size_t> RemoveOutdatedItem(const String& itemID, const String& version);

    void NotifyItemStatusChanged(const UpdateItemStatus& status);
    void NotifyItemRemovedFromCache(const String& itemID, const String& version);

    Error             LockItem(const String& itemID, const String& version);
    void              UnlockItem(const String& itemID, const String& version);
    UpdateItemStatus* FindInProgressItem(const String& itemID, const String& version);
    Error             LockBlob(const String& digest);
    bool              TryLockBlob(const String& digest);
    void              UnlockBlob(const String& digest);

    bool IsBlobInProgress(const String& digest) const
    {
        LockGuard lock {mMutex};

        return mBlobsInProgress.Find(digest) != mBlobsInProgress.end();
    }

    Config                             mConfig;
    DownloadBlobInfoProviderItf*       mBlobInfoProvider {};
    StorageItf*                        mStorage {};
    spaceallocator::SpaceAllocatorItf* mSpaceAllocator {};
    fs::FileInfoProviderItf*           mFileInfoProvider {};
    oci::OCISpecItf*                   mOCISpec {};
    fs::FSPlatformItf*                 mFSPlatform {};
    AllocatorItf*                      mAllocator {};

    Mutex                                                 mListenersMutex;
    StaticArray<ItemStatusListenerItf*, cMaxNumListeners> mListeners;

    Mutex                                                             mMutex;
    ConditionalVariable                                               mCondVar;
    StaticList<UpdateItemStatus, cMaxNumConcurrentItems>              mItemsInProgress;
    StaticList<StaticString<oci::cDigestLen>, cMaxNumConcurrentItems> mBlobsInProgress;

    ItemRemover mItemRemover {*this};
};

} // namespace aos::common::imagemanager

#endif
