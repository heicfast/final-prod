# MS-DOCS.md — Microsoft-documentation context sheet: Windows sector-read APIs

Session: `session-sector-scan-rebuild` (Agent D, sector-scan rebuild)
Date: 2026-11-18
Purpose: every platform claim made by the rebuilt sector-read path must be traceable to a real Microsoft Learn page. This sheet surveys seven topic areas and ends with the concrete platform requirements for the rebuild.

Method: every page below was fetched directly from learn.microsoft.com during this session. Quotations are verbatim from the fetched page text. Anything not present on a fetched page is marked **UNVERIFIED** (or "not on the fetched page"). No doc text has been invented. Requirements-table values are quoted as printed on the fetched pages (note: some Windows-driver DDI pages print empty minimum-OS cells).

Citation shorthand used below:
- **[QRY]** https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_query_property
- **[SPQ]** https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ns-ntddstor-_storage_property_query
- **[SPID]** https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ne-ntddstor-storage_property_id
- **[SAAD-KM]** https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ns-ntddstor-_storage_access_alignment_descriptor
- **[SDD]** https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ns-ntddstor-_storage_device_descriptor
- **[SAAD]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_access_alignment_descriptor
- **[FIBHC]** https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ne-minwinbase-file_info_by_handle_class
- **[FAI]** https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_alignment_info
- **[GFIBHE]** https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex
- **[FSI]** https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_standard_info
- **[FSTI]** https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_storage_info
- **[GEOEX-IOCTL]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry_ex
- **[GEO-IOCTL]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry
- **[DGEX]** https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ns-ntdddisk-_disk_geometry_ex
- **[DG]** https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ns-ntdddisk-_disk_geometry
- **[LEN-IOCTL]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_length_info
- **[GLI]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-get_length_information
- **[CFW]** https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
- **[LOCK]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_lock_volume
- **[DISM]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_dismount_volume
- **[RF]** https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
- **[FB]** https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering
- **[OVL]** https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-overlapped
- **[GOR]** https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult
- **[CICP]** https://learn.microsoft.com/en-us/windows/win32/fileio/createiocompletionport
- **[GQCS]** https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus
- **[IOCP]** https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports
- **[CIE]** https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex
- **[AF4K]** https://learn.microsoft.com/en-us/windows/compatibility/advanced-format-disk-compatibility-update
- **[ERR0]** https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
- **[ERR6]** https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--6000-8199-
- **[VER-IOCTL]** https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_verify
- **[SYNCASYNC]** https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o
- **[FC]** https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching

---

## 1. IOCTL_STORAGE_QUERY_PROPERTY — querying sector sizes and access alignment

**IOCTL_STORAGE_QUERY_PROPERTY** [QRY]:
- "A driver can use IOCTL_STORAGE_QUERY_PROPERTY to return properties of a storage device or adapter. The request indicates the kind of information to retrieve, such as inquiry data for a device or capabilities and limitations of an adapter."
- Input buffer: "Parameters.DeviceIoControl.InputBufferLength indicates the size, in bytes, of the parameter buffer at Irp->AssociatedIrp.SystemBuffer, which must be >= sizeof(STORAGE_PROPERTY_QUERY)." The input "specifies whether to query the device or the adapter, the type of query to perform, and any additional parameters required for the query".
- "Device properties must be retrieved only from a device; attempting to retrieve device properties from an adapter will cause an error." [QRY]
- Output sizing: "Cast the structure returned to a STORAGE_DESCRIPTOR_HEADER and check its Size member to determine the number of bytes the structure actually requires." [QRY]
- Requirements: Header ntddstor.h. (The DDI page's minimum client/server cells are blank on the fetched page.)

**STORAGE_PROPERTY_QUERY** [SPQ]: "This structure is used in conjunction with IOCTL_STORAGE_QUERY_PROPERTY to retrieve the properties of a storage device or adapter."
- Members: `PropertyId` ("Indicates whether the caller is requesting a device descriptor, an adapter descriptor, a write cache property, a device unique ID (DUID), or the device identifiers provided in the device's SCSI vital product data (VPD) page"), `QueryType` ("Contains flags indicating the type of query to be performed"), `AdditionalParameters[1]`.
- Remarks: "The results of the query can be one of several structures depending on the value of the PropertyId member." and "If the QueryType member is set to PropertyExistsQuery, then no structure is returned."
- Requirements (as printed): Minimum supported client Windows 10, Minimum supported server Windows Server 2016, Header ntddstor.h. (This is the DDI page's table; the user-mode winioctl.h usage below is documented separately on the Win32 pages.)

**STORAGE_PROPERTY_ID** [SPID]: "STORAGE_PROPERTY_ID enumerates the possible values for STORAGE_PROPERTY_QUERY's PropertyId member."
- `StorageDeviceProperty`: "The caller is querying for the device descriptor, STORAGE_DEVICE_DESCRIPTOR."
- `StorageAccessAlignmentProperty`: "The caller is querying for the access alignment descriptor, STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR."
- Requirements: Header ntddstor.h.

**STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR** (Win32, winioctl.h) [SAAD]: "Used in conjunction with the IOCTL_STORAGE_QUERY_PROPERTY control code to retrieve the storage access alignment descriptor data for a device." (The kernel-mode page [SAAD-KM] carries the same member text: "The STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR structure is used in conjunction with the IOCTL_STORAGE_QUERY_PROPERTY request to retrieve the storage access alignment descriptor data for a device.")
- `BytesPerLogicalSector` — "The number of bytes in a logical sector of the device."
- `BytesPerPhysicalSector` — "The number of bytes in a physical sector of the device."
- `BytesOffsetForSectorAlignment` — "The logical sector offset within the first physical sector where the first logical sector is placed, in bytes." (Both pages include the worked example "BytesOffsetForSectorAlignment = 3 * BytesPerLogicalSector".)
- `BytesPerCacheLine` — "The number of bytes in a cache line of the device."; `BytesOffsetForCacheAlignment` — "The address offset necessary for proper cache access alignment, in bytes."
- Requirements [SAAD]: Minimum supported client Windows Vista [desktop apps only], Minimum supported server Windows Server 2008 [desktop apps only], Header winioctl.h.
- [SAAD-KM] Remarks: "Storage class drivers issue a device-control request with the I/O control code IOCTL_STORAGE_QUERY_PROPERTY to retrieve this structure, which contains access alignment information for data transfer operations."

**STORAGE_DEVICE_DESCRIPTOR** (StorageDeviceProperty output) [SDD]: "The STORAGE_DEVICE_DESCRIPTOR structure is used in conjunction with the IOCTL_STORAGE_QUERY_PROPERTY request to retrieve the storage device descriptor data for a device." It carries inquiry-type data: VendorIdOffset/ProductIdOffset/ProductRevisionOffset/SerialNumberOffset (offsets to NULL-terminated ASCII strings), BusType, RawDeviceProperties. "Applications and storage class drivers issue a device-control request with the I/O control code IOCTL_STORAGE_QUERY_PROPERTY to retrieve this structure, which contains information about a target device. The structure can be retrieved only from an FDO; attempting to retrieve device properties from an adapter causes an error." Buffer sizing is via STORAGE_DESCRIPTOR_HEADER's Version/Size.
- **Important negative finding:** the fetched [SDD] page does **not** document a `BytesPerSector` member in STORAGE_DEVICE_DESCRIPTOR. Do not claim it does. The documented sources of the *logical* sector size are the geometry IOCTLs and GetDiskFreeSpace (see File Buffering quote below and section 2).

**How the docs say to get logical vs physical sector size (File Buffering page)** [FB]:
- "Most current Windows APIs, such as IOCTL_DISK_GET_DRIVE_GEOMETRY and GetDiskFreeSpace, will return the logical sector size, but the physical sector size can be retrieved through the IOCTL_STORAGE_QUERY_PROPERTY control code, with the relevant information contained in the BytesPerPhysicalSector member in the STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR structure."
- "Microsoft strongly recommends that developers align unbuffered I/O to the physical sector size as reported by the IOCTL_STORAGE_QUERY_PROPERTY control code to help ensure their applications are prepared for this sector size transition."
- "Windows Server 2003 and Windows XP: The STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR structure isn't available. It was introduced with Windows Vista and Windows Server 2008."

**Alignment requirement for direct (unbuffered) I/O** [FB] — "Alignment and File Access Requirements":
- "File access sizes, including the optional file offset in the OVERLAPPED structure, if specified, must be for a number of bytes that is an integer multiple of the volume sector size. For example, if the sector size is 512 bytes, an application can request reads and writes of 512, 1,024, 1,536, or 2,048 bytes, but not of 335, 981, or 7,171 bytes."
- "File access buffer addresses for read and write operations should be physical sector-aligned, which means aligned on addresses in memory that are integer multiples of the volume's physical sector size. Depending on the disk, this requirement may not be enforced."
- "Physical Sector: The unit for which read and write operations to the device are completed in a single operation. This is the unit of atomic write, and what unbuffered I/O will need to be aligned to in order to have optimal performance and reliability characteristics." [FB]
- Buffer allocation: "One way to sector-align buffers is to use the VirtualAlloc function" or `_aligned_malloc`; "in most situations, page-aligned memory will also be sector-aligned, because the case where the sector size is larger than the page size is rare." [FB]

**FileAlignmentInfo / FILE_ALIGNMENT_INFO**:
- [FAI]: "Contains alignment information for a file. This structure is returned from the GetFileInformationByHandleEx function when FileAlignmentInfo is passed in the FileInformationClass parameter." Member `AlignmentRequirement`: "Minimum alignment requirement, in bytes." Requirements: Windows 8 / Windows Server 2012, winbase.h.
- [FIBHC]: `FileAlignmentInfo` (0x11) is listed with: "File alignment information should be retrieved. Use for any handles. Use only when calling GetFileInformationByHandleEx. See FILE_ALIGNMENT_INFO." with the note "**Windows Server 2008 R2, Windows 7, Windows Server 2008, Windows Vista, Windows Server 2003 and Windows XP:** This value is not supported before Windows 8 and Windows Server 2012". [GFIBHE] confirms the class-to-structure mapping "FileAlignmentInfo (0x11) | FILE_ALIGNMENT_INFO".
- **Negative finding:** the fetched [FIBHC] page contains **no** enumeration value named `FilePhysicalSectorInformation` (or any physical-sector value). The Win8-era physical-sector query API documented by Microsoft is the `FileFsSectorSizeInformation` file-information class with `FILE_FS_SECTOR_SIZE_INFORMATION` (see section 6), not a FILE_INFO_BY_HANDLE_CLASS value.

---

## 2. IOCTL_DISK_GET_DRIVE_GEOMETRY_EX (DISK_GEOMETRY_EX) and how to get disk size

**IOCTL_DISK_GET_DRIVE_GEOMETRY_EX** [GEOEX-IOCTL]: "Retrieves extended information about the physical disk's geometry: type, number of cylinders, tracks per cylinder, sectors per track, bytes per sector, and size." Requirements: Windows XP / Windows Server 2003, winioctl.h.

**IOCTL_DISK_GET_DRIVE_GEOMETRY** [GEO-IOCTL]: "Retrieves information about the physical disk's geometry: type, number of cylinders, tracks per cylinder, sectors per track, and bytes per sector." Plus the note: "IOCTL_DISK_GET_DRIVE_GEOMETRY has been superseded by IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, which retrieves additional information." The advanced-format page [AF4K] lists IOCTL_DISK_GET_DRIVE_GEOMETRY and IOCTL_DISK_GET_DRIVE_GEOMETRY_EX among "examples of APIs that retrieve the logical sector size".

**DISK_GEOMETRY_EX** [DGEX]: "The DISK_GEOMETRY_EX structure is an arbitrary-length structure composed of a DISK_GEOMETRY structure followed by a DISK_PARTITION_INFO structure followed, in turn, by a DISK_DETECTION_INFO structure."
- `Geometry` — "See DISK_GEOMETRY for a description of this member."
- `DiskSize` — "Contains the size in bytes of the disk."  ← the documented size member.
- `Data[1]` — "Beginning of the data block, starting with a DISK_PARTITION_INFO structure followed by a DISK_DETECTION_INFO structure." Access via the `DiskGeometryGetPartition` / `DiskGeometryGetDetect` macros shown on the page.
- Remarks: "DISK_GEOMETRY_EX is used in conjunction with the IOCTL_DISK_GET_DRIVE_GEOMETRY_EX and the IOCTL_DISK_GET_MEDIA_TYPES IOCTLs, in order to retrieve information about the geometry of a physical disk (media type, number of cylinders, tracks per cylinder, sectors per track, and bytes per sector)."

**DISK_GEOMETRY** [DG]:
- "The DISK_GEOMETRY structure is obsolete and provided only to support existing drivers. New drivers must use DISK_GEOMETRY_EX."
- Members: `Cylinders` ("Indicates the number of cylinders on the disk device"), `MediaType`, `TracksPerCylinder` ("Indicates the number of tracks in a cylinder"), `SectorsPerTrack` ("Indicates the number of sectors in each track"), `BytesPerSector` ("Indicates the number of bytes in a disk sector").
- Remarks: "DISK_GEOMETRY_EX must be used with new drivers, in order to accommodate GUID Partition Table (GPT) partitions."

**Caveat about CHS not multiplying out to disk size — UNVERIFIED on the fetched pages.** None of the fetched pages ([GEOEX-IOCTL], [GEO-IOCTL], [DGEX], [DG]) states in so many words that cylinders x tracks x sectors-per-track x bytes-per-sector may not equal the true disk size on LBA-only / 4Kn / 512e disks. What the docs do support: (a) DISK_GEOMETRY is "obsolete" [DG]; (b) the size is carried by the dedicated `DiskSize` member of DISK_GEOMETRY_EX [DGEX]; (c) these IOCTLs report the *logical* sector size [FB], [AF4K]. Treat "CHS multiplication is unreliable" as a strong inference, not a doc quote; derive size only from DiskSize / GET_LENGTH_INFO.

**IOCTL_DISK_GET_LENGTH_INFO** [LEN-IOCTL]: "Retrieves the length of the specified disk, volume, or partition."
- Remarks: "Volume handles do not have access to the full volume. To read or write to the last few sectors of a volume, you must call FSCTL_ALLOW_EXTENDED_DASD_IO, which instructs the file system to not perform any boundary checks." and "This operation should be used instead of IOCTL_DISK_GET_PARTITION_INFO_EX for volumes that do not have partition info—such as partition type or number of hidden sectors."
- Requirements: Windows XP / Windows Server 2003, winioctl.h.

**GET_LENGTH_INFORMATION** [GLI]: "Contains disk, volume, or partition length information used by the IOCTL_DISK_GET_LENGTH_INFO control code." Member `Length`: "The length of the disk, volume, or partition, in bytes."

**GetFileInformationByHandleEx / FileStandardInfo** [GFIBHE], [FSI]:
- [GFIBHE]: "Retrieves file information for the specified file." The Remarks table maps "FileStandardInfo (1) | FILE_STANDARD_INFO". Requirements: Windows Vista / Windows Server 2008, winbase.h.
- [FSI] `EndOfFile`: "The end of the file." (That is the entire member description on the fetched page.)
- **UNVERIFIED:** the fetched pages do not state that GetFileInformationByHandleEx(FileStandardInfo).EndOfFile on a volume handle returns the volume length. The documented way to obtain volume/disk length is IOCTL_DISK_GET_LENGTH_INFO [LEN-IOCTL] or DISK_GEOMETRY_EX.DiskSize [DGEX]. If the rebuild uses EndOfFile on a DASD volume handle, that technique is undocumented on the pages fetched here and must be labeled as such.

**Sector-count derivation.** No fetched page states a "sector count" formula. The only doc-backed derivation is arithmetic from documented quantities: sectorCount = diskSize [DGEX] / bytesPerSector [DG or SAAD], or = length [GLI] / logicalBytesPerSector [SAAD]. Mark any such product formula as derived, not doc-quoted.

---

## 3. CreateFile on volume handles: \\.\PhysicalDriveN vs \\.\C:, locking, dismount, share flags, caching

All quotes in this section are from the "Physical Disks and Volumes" section of CreateFileW [CFW] unless noted.

**Access and restrictions:**
- "Direct access to the disk or to a volume is restricted." ("Windows Server 2003 and Windows XP: Direct access to the disk or to a volume is not restricted in this manner.")
- "You can use the CreateFile function to open a physical disk drive or a volume, which returns a direct access storage device (DASD) handle that can be used with the DeviceIoControl function. This enables you to access the disk or volume directly, for example such disk metadata as the partition table. However, this type of access also exposes the disk drive or volume to potential data loss..."
- Required for the call to succeed: "The caller must have administrative privileges." / "The dwCreationDisposition parameter must have the OPEN_EXISTING flag." / "When opening a volume or floppy disk, the dwShareMode parameter must have the FILE_SHARE_WRITE flag."

**Name forms and what they open:**
- Physical drive: "When opening a physical drive x:, the lpFileName string should be the following form: "\.\PhysicalDriveX". Hard disk numbers start at zero." Table: "\.\PhysicalDrive0 | Opens the first physical drive."
- Volume: "When opening a volume or removable media drive (for example, a floppy disk drive or flash memory thumb drive), the lpFileName string should be the following form: "\.\X:". Do not use a trailing backslash (\), which indicates the root directory of a drive." Table: "\.\C: | Opens the C: volume." / "\.\C:\ | Opens the file system of the C: volume."
- "To obtain the physical drive identifier for a volume, open a handle to the volume and call the DeviceIoControl function with IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS. This control code returns the disk number and offset for each of the volume's one or more extents; a volume can span multiple physical disks."

**Caching semantics on volume handles (quote exactly — do not overclaim):**
- "A volume contains one or more mounted file systems. Volume handles can be opened as noncached at the discretion of the particular file system, even when the noncached option is not specified in CreateFile. You should assume that all Microsoft file systems open volume handles as noncached. The restrictions on noncached I/O for files also apply to volumes."
- "A file system may or may not require buffer alignment even though the data is noncached. However, if the noncached option is specified when opening a volume, buffer alignment is enforced regardless of the file system on the volume. It is recommended on all file systems that you open volume handles as noncached, and follow the noncached I/O restrictions."
- "Note: To read or write to the last few sectors of the volume, you must call DeviceIoControl and specify FSCTL_ALLOW_EXTENDED_DASD_IO. This signals the file system driver not to perform any I/O boundary checks on partition read or write calls. Instead, boundary checks are performed by the device driver."
- Related, from File Caching [FC]: "When caching is disabled, all read and write operations directly access the physical disk. However, the file metadata may still be cached." (That page is about files; the volume-specific statement is the CreateFileW quote above.)

**FSCTL_LOCK_VOLUME** [LOCK]: "Locks a volume if it is not in use. A locked volume can be accessed only through handles to the file object (hDevice) that locks the volume."
- Handle requirements: "The hDevice handle passed to DeviceIoControl must be a handle to a volume, opened for direct access. To retrieve this handle, call CreateFile with the lpFileName parameter set to a string of the following form: \.\X: ... The application must also specify the FILE_SHARE_READ and FILE_SHARE_WRITE flags in the dwShareMode parameter of CreateFile."
- "If the specified volume is a system volume or contains a page file, the operation fails."
- "If there are any open files on the volume, this operation will fail. Conversely, success of this operation indicates there are no open files."
- "This operation is useful for applications that need exclusive access to a volume for a period of time—for example, disk utility and backup programs."
- "A locked volume remains locked until one of the following occurs: The application uses the FSCTL_UNLOCK_VOLUME control code to unlock the volume. The handle closes, either directly through CloseHandle, or indirectly when a process terminates."
- "The system flushes all cached data to the volume before locking it. For example, any data held in a lazy-write cache is written to the volume."
- "The NTFS file system treats a locked volume as a dismounted volume. The FSCTL_DISMOUNT_VOLUME control code functions similarly but does not check for open files before dismounting. Note that without a successful lock operation, a dismounted volume may be remounted by any process at any time. This would not be an ideal state for performing a volume backup, for example."

**FSCTL_DISMOUNT_VOLUME** [DISM]: "Dismounts a volume regardless of whether or not the volume is currently in use."
- "The FSCTL_DISMOUNT_VOLUME control code will attempt to dismount a volume regardless of whether or not any other processes are using the volume, which can have unpredictable results for those processes if they do not hold a lock on the volume."
- Same share-mode requirement as [LOCK]: "The application must also specify the FILE_SHARE_READ and FILE_SHARE_WRITE flags in the dwShareMode parameter of CreateFile."
- "If the specified volume is a system volume or contains a page file, the operation fails." / "If the specified volume is locked by another process, the operation fails. To prevent another process from locking the volume, lock it as soon as you open it."
- Dismounted volume properties (quoted verbatim, including the page's own wording): "There are no open files." and "The operating system does detect the volume." [page text as fetched]
- "The operating system tries to mount an unmounted volume as soon as an attempt is made to access it. For example, a call to GetLogicalDrives triggers the operating system to mount unmounted volumes."
- Documented procedure for a volume file-system change: "1. Open a volume. 2. Lock the volume. 3. Format the volume. 4. Dismount the volume. 5. Unlock the volume. 6. Close the volume handle."

**Share-flag summary (as documented):** CreateFileW requires FILE_SHARE_WRITE when opening a volume [CFW]; the FSCTL pages direct that the application "must also specify the FILE_SHARE_READ and FILE_SHARE_WRITE flags" [LOCK], [DISM]. For a read-only scanner, open with both share flags; note the docs' requirement is stated for direct-access volume handles used with these control codes.

---

## 4. FILE_FLAG_NO_BUFFERING + FILE_FLAG_OVERLAPPED semantics

**Flag descriptions (CreateFileW)** [CFW]:
- FILE_FLAG_NO_BUFFERING (0x20000000): "The file or device is being opened with no system caching for data reads and writes. This flag does not affect hard disk caching or memory mapped files. There are strict requirements for successfully working with files opened with CreateFile using the FILE_FLAG_NO_BUFFERING flag, for details see File Buffering."
- FILE_FLAG_OVERLAPPED (0x40000000): "The file or device is being opened or created for asynchronous I/O. When subsequent I/O operations are completed on this handle, the event specified in the OVERLAPPED structure will be set to the signaled state. If this flag is specified, the file can be used for simultaneous read and write operations. If this flag is not specified, then I/O operations are serialized, even if the calls to the read and write functions specify an OVERLAPPED structure."
- Caching Behavior section: "When FILE_FLAG_NO_BUFFERING is combined with FILE_FLAG_OVERLAPPED, the flags give maximum asynchronous performance, because the I/O does not rely on the synchronous operations of the memory manager. However, some I/O operations take more time, because data is not being held in the cache."

**Alignment requirements under NO_BUFFERING** [FB]: quoted in section 1 — access sizes (including the OVERLAPPED file offset) must be integer multiples of the volume sector size; buffer addresses should be physical-sector-aligned (enforcement varies: "Depending on the disk, this requirement may not be enforced"); Microsoft recommends aligning unbuffered I/O to the physical sector size reported by IOCTL_STORAGE_QUERY_PROPERTY.

**Volume handles without NO_BUFFERING — what the docs actually say** [CFW]: "Volume handles can be opened as noncached at the discretion of the particular file system, even when the noncached option is not specified in CreateFile. You should assume that all Microsoft file systems open volume handles as noncached. The restrictions on noncached I/O for files also apply to volumes." So the documented position is: assume volume handles are noncached regardless, and Microsoft recommends explicitly opening them noncached and following the noncached I/O restrictions. (Do not claim the docs say raw volume reads "bypass the cache" in some stronger sense — that sentence is the whole documented statement.)

**ReadFile rules** [RF]:
- "Reads data from the specified file or input/output (I/O) device. Reads occur at the position specified by the file pointer if supported by the device."
- Return value: "If the function fails, or is completing asynchronously, the return value is zero (FALSE)." Note: "The GetLastError code ERROR_IO_PENDING is not a failure; it designates the read operation is pending completion asynchronously."
- Buffer lifetime: "This buffer must remain valid for the duration of the read operation. The caller must not use this buffer until the read operation is completed."
- With FILE_FLAG_OVERLAPPED: "The lpOverlapped parameter must point to a valid and unique OVERLAPPED structure, otherwise the function can incorrectly report that the read operation is complete." and "The lpNumberOfBytesRead parameter should be set to NULL. Use the GetOverlappedResult function to get the actual number of bytes read. If the hFile parameter is associated with an I/O completion port, you can also get the number of bytes read by calling the GetQueuedCompletionStatus function." ("Windows 7: This parameter can not be NULL.")
- Offset: "For an hFile that supports byte offsets, if you use this parameter you must specify a byte offset at which to start reading from the file or device. This offset is specified by setting the Offset and OffsetHigh members of the OVERLAPPED structure."
- Pending state discipline: "ReadFile may return before the read operation is complete. In this scenario, ReadFile returns FALSE and the GetLastError function returns ERROR_IO_PENDING... ReadFile resets the event to a nonsignaled state when it begins the I/O operation... until that time, the read operation is considered pending." and "neither the offset nor any other part of the structure should be modified, freed, or reused by the application until the event is signaled (that is, the read completes)."
- "The ReadFile function may fail with ERROR_INVALID_USER_BUFFER or ERROR_NOT_ENOUGH_MEMORY whenever there are too many outstanding asynchronous I/O requests."
- "If part of a file is locked by another process and the read operation overlaps the locked portion, this function fails."

**OVERLAPPED structure** [OVL]:
- `Offset` / `OffsetHigh`: "The low-order portion of the file position at which to start the I/O request, as specified by the user." / "The high-order portion of the file position at which to start the I/O request, as specified by the user."
- Remarks: "The Offset and OffsetHigh members together represent a 64-bit file position. It is a byte offset from the start of the file or file-like device, and it is specified by the user; the system will not modify these values. The calling process must set this member before passing the OVERLAPPED structure to functions that use an offset, such as the ReadFile or WriteFile (and related) functions."
- hEvent: manual-reset event recommended — "you should use a manual reset event; if you use an auto-reset event, your application can stop responding if you wait for the operation to complete and then call GetOverlappedResult with the bWait parameter set to TRUE."
- "A common mistake is to reuse an OVERLAPPED structure before the previous asynchronous operation has been completed. You should use a separate structure for each request."

**File pointer not used / concurrent overlapped operations on one handle:**
- [SYNCASYNC]: "The system does not maintain the file pointer on asynchronous handles to files and devices that support file pointers (that is, seeking devices), therefore the file position must be passed to the read and write functions in the related offset data members of the OVERLAPPED structure."
- [CFW]: "If this flag [FILE_FLAG_OVERLAPPED] is specified, the file can be used for simultaneous read and write operations. If this flag is not specified, then I/O operations are serialized, even if the calls to the read and write functions specify an OVERLAPPED structure."
- [SYNCASYNC] caution about waiting on the file handle: "if an application starts two I/O operations and waits on the file handle, there is no way to determine which operation is finished when the handle is set to the signaled state. If an application must perform multiple asynchronous I/O operations on a single file, it should wait on the event handle in the specific OVERLAPPED structure for each I/O operation, rather than on the common file handle."
- [SYNCASYNC] also notes async calls can behave synchronously: "subsequent calls to functions such as WriteFile using that handle generally return immediately but can also behave synchronously with respect to blocked execution."

**GetOverlappedResult** [GOR]:
- "Retrieves the results of an overlapped operation on the specified file, named pipe, or communications device."
- bWait: "If this parameter is TRUE, and the Internal member of the lpOverlapped structure is STATUS_PENDING, the function does not return until the operation has been completed. If this parameter is FALSE and the operation is still pending, the function returns FALSE and the GetLastError function returns ERROR_IO_INCOMPLETE."
- hEvent == NULL: "If the hEvent member of the OVERLAPPED structure is NULL, the system uses the state of the hFile handle to signal when the operation has completed. Use of file, named pipe, or communications-device handles for this purpose is discouraged. It is safer to use an event object because of the confusion that can occur when multiple simultaneous overlapped operations are performed on the same file, named pipe, or communications device."
- "The results reported by the GetOverlappedResult function are those of the specified handle's last overlapped operation to which the specified OVERLAPPED structure was provided, and for which the operation's results were pending."

---

## 5. I/O completion ports: association, dequeue order, thread pools, CancelIoEx

**CreateIoCompletionPort** [CICP]:
- "Creates an input/output (I/O) completion port and associates it with a specified file handle, or creates an I/O completion port that is not yet associated with a file handle, allowing association at a later time."
- FileHandle: "The handle must be to an object that supports overlapped I/O. If a handle is provided, it has to have been opened for overlapped I/O completion. For example, you must specify the FILE_FLAG_OVERLAPPED flag when using the CreateFile function to obtain the handle." INVALID_HANDLE_VALUE creates a port with no association (ExistingCompletionPort must be NULL, CompletionKey ignored).
- NumberOfConcurrentThreads: "The maximum number of threads that the operating system can allow to concurrently process I/O completion packets for the I/O completion port... If this parameter is zero, the system allows as many concurrently running threads as there are processors in the system."
- "Multiple file handles can be associated with a single I/O completion port by calling CreateIoCompletionPort multiple times with the same I/O completion port handle in the ExistingCompletionPort parameter and a different file handle in the FileHandle parameter each time."
- "A handle can be associated with only one I/O completion port, and after the association is made, the handle remains associated with that I/O completion port until it is closed."
- CompletionKey: "This value is not used by CreateIoCompletionPort for functional control; rather, it is attached to the file handle specified in the FileHandle parameter at the time of association... It is returned in the GetQueuedCompletionStatus function call when a completion packet arrives."
- "After an instance of an open handle is associated with an I/O completion port, it cannot be used in the ReadFileEx or WriteFileEx function because these functions have their own asynchronous I/O mechanisms."
- Lifetime: "The I/O completion port handle and every file handle associated with that particular I/O completion port are known as references to the I/O completion port. The I/O completion port is released when there are no more references to it. Therefore, all of these handles must be properly closed to release the I/O completion port and its associated system resources."
- Requirements: Windows XP / Windows Server 2003; IoAPI.h; Kernel32.lib/dll.

**I/O Completion Ports (concept)** [IOCP]:
- "I/O completion ports provide an efficient threading model for processing multiple asynchronous I/O requests on a multiprocessor system."
- Queue order (the ordering statement the rebuild must respect): "When an asynchronous I/O operation on one of these file handles completes, an I/O completion packet is queued in first-in-first-out (FIFO) order to the associated I/O completion port... Please note that while the packets are queued in FIFO order they may be dequeued in a different order."
- Thread release order: "Threads that block their execution on an I/O completion port are released in last-in-first-out (LIFO) order, and the next completion packet is pulled from the I/O completion port's FIFO queue for that thread. This means that, when a completion packet is released to a thread, the system releases the last (most recent) thread associated with that port, passing it the completion information for the oldest I/O completion."
- Status-block update timing (critical for the rebuild): "When a file handle is associated with a completion port, the status block passed in will not be updated until the packet is removed from the completion port. The only exception is if the original operation returns synchronously with an error."
- Concurrency: "This value limits the number of runnable threads associated with the completion port." ... "The best overall maximum value to pick for the concurrency value is the number of CPUs on the computer." ... "a good rule of thumb is to have a minimum of twice as many threads in the thread pool as there are processors on the system."
- Thread-pool guidance: the page's "When to use I/O completion ports" table recommends thread pool I/O (`CreateThreadpoolIo`) for moderate concurrency: "The Windows thread pool API (CreateThreadpoolIo, StartThreadpoolIo) uses IOCP internally but handles thread lifecycle management automatically... Use raw IOCP when you need explicit control over the completion port's concurrency value or custom thread management."
- Supported I/O functions list includes ReadFile, WriteFile and DeviceIoControl. [IOCP]

**GetQueuedCompletionStatus** [GQCS]:
- "Attempts to dequeue an I/O completion packet from the specified I/O completion port. If there is no completion packet queued, the function waits for a pending I/O operation associated with the completion port to complete."
- Timeout: "If a completion packet does not appear within the specified time, the function times out, returns FALSE, and sets *lpOverlapped to NULL." dwMilliseconds == INFINITE never times out; zero times out immediately if nothing is queued.
- Port closed while waiting: "If a call to GetQueuedCompletionStatus fails because the completion port handle associated with it is closed while the call is outstanding, the function returns FALSE, *lpOverlapped will be NULL, and GetLastError will return ERROR_ABANDONED_WAIT_0."
- Failed I/O dequeue: "If *lpOverlapped is not NULL and the function dequeues a completion packet for a failed I/O operation from the completion port, the function stores information about the failed operation in the variables pointed to by lpNumberOfBytes, lpCompletionKey, and lpOverlapped. To get extended error information, call GetLastError."
- Packet suppression: "an application can prevent completion port notification. This is done by specifying a valid event handle for the hEvent member of the OVERLAPPED structure, and setting its low-order bit."
- "This function associates a thread with the specified completion port. A thread can be associated with at most one completion port."

**CancelIoEx** [CIE]:
- "Marks any outstanding I/O operations for the specified file handle. The function only cancels I/O operations in the current process, regardless of which thread created the I/O operation."
- Specific-operation cancel: "If this parameter [lpOverlapped] is NULL, all I/O requests for the hFile parameter are canceled. If this parameter is not NULL, only those specific I/O requests that were issued for the file with the specified lpOverlapped overlapped structure are marked as canceled, meaning that you can cancel one or more requests, while the CancelIo function cancels all outstanding requests on a file handle."
- Wait-for-completion requirement (as documented): "The application must not free or reuse the OVERLAPPED structure associated with the canceled I/O operations until they have completed. The thread can use the GetOverlappedResult function to determine when the I/O operations themselves have been completed."
- "CancelIoEx cancels only outstanding I/O on the handle, it does not change the state of the handle; this means that you cannot rely on the state of the handle because you cannot know whether the operation was completed successfully or canceled." / "The CancelIoEx function does not wait for all canceled operations to complete."
- Not-found case: "If this function cannot find a request to cancel, the return value is 0 (zero), and GetLastError returns ERROR_NOT_FOUND."
- Completion statuses: "The operation being canceled is completed with one of three statuses... The operation completed normally... The operation was canceled. The GetLastError function returns ERROR_OPERATION_ABORTED. The operation failed with another error..."
- IOCP interaction: "If the file handle is associated with a completion port, an I/O completion packet is not queued to the port if a synchronous operation is successfully canceled. For asynchronous operations still pending, the cancel operation will queue an I/O completion packet."
- Requirements: Windows Vista / Windows Server 2008.
- **Precision note:** the task brief phrased this as "all canceled I/Os complete before the handle can be safely closed". The fetched [CIE] page states the OVERLAPPED/buffer reuse rule quoted above; it does not contain that exact handle-close sentence. The rebuild should implement the documented rule (wait for completion of canceled I/O — via GetOverlappedResult or the IOCP packet — before freeing OVERLAPPED/buffers or closing handles) and not attribute the stronger phrasing to the docs.
- Related, from ReadFile [RF]: "I/O operations that are canceled complete with the error ERROR_OPERATION_ABORTED."

---

## 6. Physical sector size: Win7-era vs Win8+ APIs, and guidance for unbuffered I/O

**The compatibility problem** [AF4K] ("Advanced format disk compatibility update - Compatibility Cookbook"; platforms listed: Windows XP through Windows 8, Windows Server 2003 through Server 2016):
- "it is no longer safe to make any assumptions regarding the sector size of modern storage devices, and developers will need to study the assumptions underlying their code to determine if there is an impact."
- Logical vs physical: "Logical sector: The unit that is used for logical block addressing for the media. We can also think of it as the smallest unit of write that the storage can accept. This is the 'emulation.' Physical sector: The unit for which read and write operations to the device are completed in a single operation. This is the unit of atomic write."
- 4Kn failure mode: "4 KB native: This media has no emulation layer and directly exposes 4 KB as its logical and physical sector size. The overall issue with this new type of media is that the majority of apps and operating systems do not query for and align I/Os to the physical sector size, which can result in unexpected failed I/Os."
- 512e failure mode: "512-byte emulation (512e): ... the majority of app and operating systems do not understand the existence of the physical sector size, which can result in a number of issues."
- Support table (official Microsoft support policy for media, as printed): 512n supported on "All Windows versions"; 512e supported on "Windows 8, Windows Server 2012, Windows 7 w/ MS KB 982018, Windows 7 SP1, Windows Server 2008 R2 w/ MS KB 982018, Windows Server 2008 R2 SP1, Windows Vista w/ MS KB 2553708, Windows Server 2008 w/ MS KB 2553708"; 4Kn supported on "Windows 8, Windows Server 2012".
- "Windows XP, Windows Server 2003, and Windows Server 2003 R2 do not support 512e or 4Kn media... Microsoft strongly cautions against using 512e media with Windows XP or other products based on the Windows XP codebase."
- Read-modify-write resiliency: on 512e, a partial physical-sector overwrite "could have been corrupted with incomplete info... you can think of it as potentially having lost all 8 logical sectors (which the physical sector logically contains)." Apps must "Query for the physical sector size" and "Ensure writes are aligned to that reported physical sector size".
- Unbuffered writes: "The simplest issue is that unbuffered writes are not aligned to the reported physical sector size of the storage media... most apps with a data store perform unbuffered writes, and thus will need to ensure these writes are performed in units of the physical sector size." Also: "if you are currently aligning the writes to the sector size, this sector size is most likely just the logical sector size... The sector size of interest here is the physical sector size, which is the real unit of atomicity."
- APIs that return only the logical size (listed on [AF4K]): GetDiskFreeSpace/GetDiskFreeSpaceEx, FileFsVolumeInformation, IOCTL_DISK_GET_DRIVE_GEOMETRY, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, IVdsDisk::GetProperties / IVdsDisk3::GetProperties2.
- **Preferred method for Windows 8** [AF4K]: "Microsoft has introduced a new API... in the form of a new info class, FileFsSectorSizeInformation, with associated structure FILE_FS_SECTOR_SIZE_INFORMATION" with members LogicalBytesPerSector, PhysicalBytesPerSectorForAtomicity, PhysicalBytesPerSectorForPerformance, FileSystemEffectivePhysicalBytesPerSectorForAtomicity, Flags, ByteOffsetForSectorAlignment, ByteOffsetForPartitionAlignment. Advantages as listed: "Calling from an unprivileged app. Calling to any valid file handle. Calling to a file handle on a remote volume over SMB2. Simplified programming model."
- **Legacy method for Windows 7 and Windows Vista** [AF4K]: IOCTL_STORAGE_QUERY_PROPERTY with the STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR code sample, plus sanity checks on the reported value: physical >= logical else use logical; must be a power of two; between 512 and 4 KB consider rounding down to logical; greater than 4 KB evaluate before use. Limitations listed: "Requires elevated privilege... Does not support SMB volumes... Cannot be issued to any file handle (the IOCTL must be issued to a Volume Handle)."
- The [AF4K] page also documents `fsutil fsinfo ntfsinfo <drive letter>` for users, and notes "If the 'Byte Per Physical Sector' field displays 'Not Supported' then either the storage driver does not support IOCTL_STORAGE_QUERY_PROPERTY, or there was an error in retrieving the info."

**FILE_STORAGE_INFO (FileStorageInfo, Win8+)** [FSTI] — the GetFileInformationByHandleEx class that surfaces the same data per handle [GFIBHE maps FileStorageInfo (0x10) to FILE_STORAGE_INFO; FIBHC marks it "not supported before Windows 8 and Windows Server 2012"]:
- `LogicalBytesPerSector` — "Logical bytes per sector reported by physical storage. This is the smallest size for which uncached I/O is supported."
- `PhysicalBytesPerSectorForAtomicity` — "Bytes per sector for atomic writes. Writes smaller than this may require a read before the entire block can be written atomically."
- `PhysicalBytesPerSectorForPerformance` — "Bytes per sector for optimal performance for writes."
- `FileSystemEffectivePhysicalBytesPerSectorForAtomicity` — "the size of the block used for atomicity by the file system."
- Flags: STORAGE_INFO_FLAGS_ALIGNED_DEVICE ("logical sectors of the storage device are aligned to physical sector boundaries"), STORAGE_INFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE.
- Remarks: "If a volume is built on top of storage devices with different properties (for example a mirrored, spanned, striped, or RAID configuration) the sizes returned are those of the largest size of any of the underlying storage devices."
- Requirements: Windows 8 / Windows Server 2012, winbase.h.

**Win7 vs Win10/11 platform delta (doc-cited parts):**
- On Windows 7: FileAlignmentInfo and FileStorageInfo are "not supported before Windows 8 and Windows Server 2012" [FIBHC]; STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR exists since Vista/2008 [SAAD] (and "isn't available" on Server 2003/XP [FB]); 512e needs Win7 SP1 or KB 982018; 4Kn is not supported on Win7 [AF4K]. The legacy query path (IOCTL_STORAGE_QUERY_PROPERTY to a volume handle, elevated) is the documented Win7 method [AF4K].
- On Windows 8+ (and by extension the modern stack): FileFsSectorSizeInformation / FILE_FS_SECTOR_SIZE_INFORMATION [AF4K] and FileStorageInfo/FILE_STORAGE_INFO [FSTI] are the documented per-handle queries usable by unprivileged apps on any file handle.
- **UNVERIFIED:** no Windows-10/11-specific physical-sector guidance page was fetched in this session (the fetched [AF4K] page is the Win8-era compatibility article, listing platforms up to Windows 8 / Server 2016). Any claim specific to "Windows 10/11 changed X about 4K" must not be cited to the pages above. A related support-policy page exists in search results (https://learn.microsoft.com/en-us/troubleshoot/windows-server/backup-and-storage/support-policy-4k-sector-hard-drives) but was **not fetched** in this session — do not cite it.

**"1 MB, or a multiple of the physical sector size" style guidance — NOT FOUND on the fetched pages.** The File Buffering [FB] and File Caching [FC] pages fetched here contain no 1 MB buffer-size recommendation. What [FC] does say: "The amount of I/O performance improvement that file data caching offers depends on the size of the file data block being read or written. When large blocks of file data are read and written, it is more likely that disk reads and writes will be necessary to finish the I/O operation." And [FB] gives only the sector-multiple example (512 → 512/1024/1536/2048). If the rebuild cites a "1 MB or multiple of physical sector size" rule, that citation is **UNVERIFIED** against this session's fetches.

---

## 7. ReadFile error behavior with partial data; error codes; IOCTL_DISK_VERIFY; "ReadFileGraded"

**ReadFile documented return conditions** [RF]: "The ReadFile function returns when one of the following conditions occur: The number of bytes requested is read. A write operation completes on the write end of the pipe. An asynchronous handle is being used and the read is occurring asynchronously. An error occurs."

**Partial data at end of file (documented)** [RF]: "If a read operation on a file begins at or beyond the end of the file, then the read operation fails with the error ERROR_HANDLE_EOF. If a read operation on a file begins before the end of the file, but the read operation extends past the end of the file, then the read operation succeeds, and the number of bytes read is the number of bytes that were read before the end of file was reached." This is the only partial-read behavior documented on the fetched ReadFile page.

**Partial data on device/media errors — UNVERIFIED.** The fetched [RF] page does not document how many bytes (if any) are transferred when a read on a volume/physical-drive handle fails with a media error. Any claim like "on ERROR_CRC you may still get N bytes" is not on the fetched pages and must be treated as unverified; classify by return value + GetLastError and re-read at finer granularity only as an implementation heuristic, not as documented behavior.

**Error-code texts (as printed on the fetched WinError.h listings):**
- [ERR0] System Error Codes (0-499):
  - ERROR_CRC — 23 (0x17) — "Data error (cyclic redundancy check)."
  - ERROR_SECTOR_NOT_FOUND — 27 (0x1B) — "The drive cannot find the sector requested."
  - ERROR_WRITE_FAULT — 29 (0x1D) — "The system cannot write to the specified device."
  - ERROR_READ_FAULT — 30 (0x1E) — "The system cannot read from the specified device."
  - ERROR_GEN_FAILURE — 31 (0x1F) — "A device attached to the system is not functioning."
  - ERROR_DIRECT_ACCESS_HANDLE — 130 (0x82) — "Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O."
- [ERR6] System Error Codes (6000-8199):
  - ERROR_BAD_CLUSTERS — 6849 (0x1AC1) — "The operation could not be completed due to bad clusters on disk."
- Scope caveat: these are the generic WinError.h lists; neither fetched page ties ERROR_READ_FAULT / ERROR_CRC / ERROR_BAD_CLUSTERS specifically to ReadFile on raw volume handles. The mapping "media error on sector read surfaces as code X" is **UNVERIFIED** beyond the literal code texts above.

**IOCTL_DISK_VERIFY** [VER-IOCTL]: "Verifies the specified extent on a fixed disk." The page shows the DeviceIoControl call (input buffer + size, no output buffer) and Requirements (Windows XP / Windows Server 2003, winioctl.h). The fetched page text contains no remarks about media-error status, privilege, or VERIFY_INFORMATION member semantics — do not cite any of those details to this page. **UNVERIFIED:** whether IOCTL_DISK_VERIFY can substitute for a read to distinguish unreadable sectors (not on the fetched page).

**"ReadFileGraded" — not a real Microsoft API.** A site search of learn.microsoft.com for "ReadFileGraded" returned no results, and no such function appears on any fetched page (the ReadFile page [RF] references only ReadFileEx as the asynchronous-only variant: "For a similar function designed solely for asynchronous operation, see ReadFileEx."). Exclude it from the rebuild. (ReadFileScatter/ReadFileEx were not surveyed in this session — no claims made.)

---

## Platform requirements for the rebuilt read path

Each rule below is doc-cited; the citations are the fetched URLs from this sheet.

1. **Alignment source — query, never assume.** Get the physical sector size via IOCTL_STORAGE_QUERY_PROPERTY with STORAGE_PROPERTY_QUERY{PropertyId = StorageAccessAlignmentProperty} and read STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR.BytesPerPhysicalSector; get the logical size from BytesPerLogicalSector (or geometry). "Microsoft strongly recommends that developers align unbuffered I/O to the physical sector size as reported by the IOCTL_STORAGE_QUERY_PROPERTY control code."
   Citations: [FB] https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering ; [SPID] https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ne-ntddstor-storage_property_id ; [SAAD] https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_access_alignment_descriptor ; [QRY] https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_query_property

2. **Sanity-check the reported physical sector size before use** (physical >= logical, power of two, else fall back to logical), because "it has been observed that some drivers may not return correctly formatted data".
   Citation: [AF4K] https://learn.microsoft.com/en-us/windows/compatibility/advanced-format-disk-compatibility-update

3. **On Windows 8+ prefer the per-handle queries; on Windows 7 use the legacy IOCTL.** Win8: FileFsSectorSizeInformation / FILE_FS_SECTOR_SIZE_INFORMATION (any file handle, unprivileged) or GetFileInformationByHandleEx(FileStorageInfo) → FILE_STORAGE_INFO (LogicalBytesPerSector = "smallest size for which uncached I/O is supported"; PhysicalBytesPerSectorForAtomicity). Win7: the IOCTL path requires a volume handle and elevation; FileAlignmentInfo/FileStorageInfo are unsupported before Windows 8.
   Citations: [AF4K] (above) ; [FSTI] https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_storage_info ; [FIBHC] https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ne-minwinbase-file_info_by_handle_class ; [FAI] https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_alignment_info

4. **Alignment rule for every unbuffered read.** Buffer address, transfer length, and file offset (OVERLAPPED.Offset/OffsetHigh) must be sector-aligned: "File access sizes, including the optional file offset in the OVERLAPPED structure... must be for a number of bytes that is an integer multiple of the volume sector size" and "File access buffer addresses... should be physical sector-aligned". Allocate buffers with VirtualAlloc or _aligned_malloc.
   Citation: [FB] https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering

5. **Size/geometry source.** Take disk size from DISK_GEOMETRY_EX.DiskSize ("Contains the size in bytes of the disk") via IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, or disk/volume/partition length from IOCTL_DISK_GET_LENGTH_INFO → GET_LENGTH_INFORMATION.Length. Do not compute size from Cylinders x TracksPerCylinder x SectorsPerTrack x BytesPerSector (DISK_GEOMETRY is documented "obsolete"; the CHS-multiplication caveat itself is UNVERIFIED on the fetched pages — but the dedicated size members are the documented source). Derive sector count as size / bytes-per-sector and label it derived, not doc-quoted.
   Citations: [DGEX] https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ns-ntdddisk-_disk_geometry_ex ; [GEOEX-IOCTL] https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry_ex ; [DG] https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ns-ntdddisk-_disk_geometry ; [LEN-IOCTL] https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_length_info ; [GLI] https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-get_length_information

6. **Handle open rules.** Open the target as "\\.\PhysicalDriveN" (whole disk) or "\\.\X:" (volume, no trailing backslash) with OPEN_EXISTING, from an elevated process, with FILE_SHARE_READ | FILE_SHARE_WRITE in dwShareMode ("the dwShareMode parameter must have the FILE_SHARE_WRITE flag" [CFW]; the FSCTL pages direct both flags).
   Citations: [CFW] https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew ; [LOCK] https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_lock_volume ; [DISM] https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_dismount_volume

7. **Open volume handles noncached.** Pass FILE_FLAG_NO_BUFFERING (with FILE_FLAG_OVERLAPPED): "It is recommended on all file systems that you open volume handles as noncached, and follow the noncached I/O restrictions." Even without the flag, "You should assume that all Microsoft file systems open volume handles as noncached."
   Citation: [CFW] (above)

8. **Reach the last sectors with FSCTL_ALLOW_EXTENDED_DASD_IO.** "To read or write to the last few sectors of the volume, you must call DeviceIoControl and specify FSCTL_ALLOW_EXTENDED_DASD_IO." Also required per [LEN-IOCTL] remarks.
   Citations: [CFW], [LEN-IOCTL] (above)

9. **Exclusive-scan discipline (only when the design needs it).** For exclusive volume access: FSCTL_LOCK_VOLUME (fails if any file is open, or on system/pagefile volumes; system flushes cached data before locking), then FSCTL_DISMOUNT_VOLUME (fails if another process holds a lock); without a successful lock "a dismounted volume may be remounted by any process at any time". Unlock (FSCTL_UNLOCK_VOLUME) and close the handle to release.
   Citations: [LOCK], [DISM] (above)

10. **Async read lifecycle.** One unique OVERLAPPED per request, Offset/OffsetHigh set per read ("the system will not modify these values"; the system "does not maintain the file pointer on asynchronous handles"); treat FALSE + ERROR_IO_PENDING as pending, not failure; set lpNumberOfBytesRead to NULL (Win7: cannot be NULL — pass a real variable); do not modify, free, or reuse the OVERLAPPED structure or the buffer until the read completes; harvest results with GetOverlappedResult (bWait semantics; ERROR_IO_INCOMPLETE) or the IOCP dequeue.
    Citations: [RF] https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile ; [OVL] https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-overlapped ; [GOR] https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult ; [SYNCASYNC] https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o

11. **IOCP lifecycle.** Create the port (INVALID_HANDLE_VALUE), associate the FILE_FLAG_OVERLAPPED handle with a per-handle CompletionKey; a handle stays associated until closed and can join only one port. Concurrency value = number of CPUs (or 0). Packets are queued FIFO but "may be dequeued in a different order"; threads are released LIFO; the OVERLAPPED status block is not updated until the packet is dequeued. Let the completion-path code be solely responsible for freeing OVERLAPPED/buffer structures (avoids the documented double-free pitfall when an operation completes synchronously). Close all associated file handles and the port handle to release the port.
    Citations: [CICP] https://learn.microsoft.com/en-us/windows/win32/fileio/createiocompletionport ; [IOCP] https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports ; [GQCS] https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus ; [SYNCASYNC] (above)

12. **Cancel semantics.** Cancel a specific outstanding read with CancelIoEx(hFile, lpOverlapped) (NULL cancels all on the handle; the call "only cancels I/O operations in the current process"); completion then arrives with ERROR_OPERATION_ABORTED (or the operation may complete normally, or fail with another error — check the completion status). "The application must not free or reuse the OVERLAPPED structure associated with the canceled I/O operations until they have completed"; CancelIoEx "does not wait for all canceled operations to complete". Expect ERROR_NOT_FOUND if the operation already finished.
    Citation: [CIE] https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex ; [RF] (above)

13. **Error classification (documented texts only).** Classify failed reads by GetLastError using the documented texts — ERROR_CRC 23 "Data error (cyclic redundancy check)", ERROR_READ_FAULT 30 "The system cannot read from the specified device.", ERROR_BAD_CLUSTERS 6849 "The operation could not be completed due to bad clusters on disk.", plus ERROR_SECTOR_NOT_FOUND 27 / ERROR_GEN_FAILURE 31 — and do not claim doc-backed partial-transfer semantics for media errors (UNVERIFIED). ReadFileGraded does not exist; do not reference it. IOCTL_DISK_VERIFY exists ("Verifies the specified extent on a fixed disk") but the fetched page documents no error-reporting remarks, so any verify-based bad-sector probing is UNVERIFIED.
    Citations: [ERR0] https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499- ; [ERR6] https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--6000-8199- ; [VER-IOCTL] https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_verify

---

## Fetch log (pages opened this session, all HTTP 200)

1. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ni-ntddstor-ioctl_storage_query_property
2. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ns-ntddstor-_storage_property_query
3. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ne-ntddstor-storage_property_id
4. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ns-ntddstor-_storage_access_alignment_descriptor
5. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddstor/ns-ntddstor-_storage_device_descriptor
6. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_access_alignment_descriptor
7. https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ne-minwinbase-file_info_by_handle_class
8. https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_alignment_info
9. https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getfileinformationbyhandleex
10. https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_standard_info
11. https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_storage_info
12. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry_ex
13. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_drive_geometry
14. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ns-ntdddisk-_disk_geometry_ex
15. https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntdddisk/ns-ntdddisk-_disk_geometry
16. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_get_length_info
17. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-get_length_information
18. https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew
19. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_lock_volume
20. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-fsctl_dismount_volume
21. https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
22. https://learn.microsoft.com/en-us/windows/win32/fileio/file-buffering
23. https://learn.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-overlapped
24. https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult
25. https://learn.microsoft.com/en-us/windows/win32/fileio/createiocompletionport
26. https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus
27. https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports
28. https://learn.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-cancelioex
29. https://learn.microsoft.com/en-us/windows/compatibility/advanced-format-disk-compatibility-update
30. https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
31. https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--6000-8199-
32. https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ni-winioctl-ioctl_disk_verify
33. https://learn.microsoft.com/en-us/windows/win32/fileio/synchronous-and-asynchronous-i-o
34. https://learn.microsoft.com/en-us/windows/win32/fileio/file-caching

URLs attempted but not fetched (HTTP 404, corrected variants above were fetched instead): `ns-ntddstor-storage_property_query`, `ns-ntddstor-storage_access_alignment_descriptor`, `ns-ntddstor-storage_device_descriptor` (missing leading underscore in DDI path); `winbase/ne-winbase-file_info_by_handle_class` (correct namespace is minwinbase); `fileio/fsctl-lock-volume` (correct namespace is winioctl). Not fetched, do not cite: the troubleshoot support-policy-4k-sector-hard-drives page found only via search.
