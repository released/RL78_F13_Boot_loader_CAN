#ifndef CAN_BOOT_CONFIG_H
#define CAN_BOOT_CONFIG_H

/*
 * CAN boot protocol configuration shared by the boot transport and the
 * RS-CAN Lite acceptance rule. The application includes this authoritative
 * contract; its local IRQ handler mapping never changes the boot image.
 */
#define CAN_BOOT_REQUEST_ID                 (0x700U)
#define CAN_BOOT_RESPONSE_ID                (0x701U)

#define CAN_BOOT_PROTOCOL_ID                (0x0001U)
#define CAN_BOOT_PROTOCOL_VERSION           (0x05U)
#define CAN_BOOT_BLOCK_BYTES                (64U)
#define CAN_BOOT_BLOCK_DATA_ID              (0x720U)
#define CAN_BOOT_CMD_BLOCK_BEGIN            (0x0BU)
#define CAN_BOOT_CMD_BLOCK_COMMIT           (0x0CU)
#define CAN_BOOT_IMAGE_FORMAT_VERSION       (0x06U)
#define CAN_BOOT_TARGET_ID                  (0xF13BU)
#define CAN_BOOT_APPLICATION_START          (0x02000UL)
#define CAN_BOOT_APPLICATION_CAPACITY       (0x1E000UL)
#define CAN_BOOT_CODE_FLASH_SIZE            (0x20000UL)
#define CAN_BOOT_RESERVED_SIZE              (0x02000UL)
#define CAN_BOOT_ERASE_BLOCK_SIZE           (0x00400UL)
#define CAN_BOOT_MANIFEST_OFFSET             (0x00100UL)
#define CAN_BOOT_MINIMUM_IMAGE_SIZE          (0x00204UL)

/*
 * Boot/app no-init RAM contract at 0xFF710.  The owner byte selects the
 * shared ISR destination.  The two signatures are one-shot launch evidence:
 * the bootloader writes HANDOFF immediately before returning to the app and
 * writes UPDATE_COMPLETE only after a successful CAN programming operation.
 * The app consumes and clears both signatures during startup.
 */
#define CAN_BOOT_SHARED_RAM_ADDRESS          (0x000FF710UL)
#define CAN_BOOT_SHARED_RAM_SIZE             (16U)
#define CAN_BOOT_RAM_OWNER_OFFSET            (0U)
#define CAN_BOOT_RAM_HANDOFF_OFFSET          (4U)
#define CAN_BOOT_RAM_UPDATE_OFFSET           (8U)
#define CAN_BOOT_RAM_OWNER_BOOT              (0xAAU)
#define CAN_BOOT_RAM_OWNER_APPLICATION       (0x55U)
#define CAN_BOOT_HANDOFF_SIGNATURE           (0x46444E48UL) /* "HNDF" */
#define CAN_BOOT_UPDATE_COMPLETE_SIGNATURE   (0x44505543UL) /* "CUPD" */
#define CAN_BOOT_RESET_TO_BOOT_SIGNATURE     (0xAA55AA55UL)

/*
 * APPLICATION_CAPACITY is a limit, not an image-size requirement.  The app
 * post-build writes its map-derived actual length into the manifest and puts
 * CRC32 in the final four bytes of that actual image.  A 60-KiB or 110-KiB
 * image is therefore valid when it fits this device geometry and all metadata
 * matches.  When repartitioning flash, change boot config, app ftable/linker
 * sections, CS+ divided-output range, and INFO/manifest declarations together.
 * Never accept a PC override as permission to program a different address.
 */

/* CAN Upgrade v5 transport; image format6 and fixed IRQ ABI are unchanged. */
#define CAN_BOOT_CHECKSUM_CRC32              (0x01U)
#define CAN_BOOT_CAN_MODE_CLASSICAL          (0x01U)
#define CAN_BOOT_CAN_ID_STANDARD             (0x00U)
#define CAN_BOOT_NOMINAL_500K_MASK           (0x00000004UL)
#define CAN_BOOT_DATA_BITRATE_MASK           (0x00000000UL)

/* Little-endian ASCII: "R5F10BMG" and "RL78/F13". */
#define CAN_BOOT_MCU_PART_0                  (0x31463552UL)
#define CAN_BOOT_MCU_PART_1                  (0x474D4230UL)
#define CAN_BOOT_MCU_PART_2                  (0x00000000UL)
#define CAN_BOOT_MCU_PART_3                  (0x00000000UL)
#define CAN_BOOT_MCU_FAMILY_0                (0x38374C52UL)
#define CAN_BOOT_MCU_FAMILY_1                (0x3331462FUL)
#define CAN_BOOT_MCU_FAMILY_2                (0x00000000UL)

/*
 * Non-destructive INFO identity.  The request is [0x07, page] and the
 * eight-byte response is [0x07, status, page, state, value32 little-endian].
 * Page 0 packs target ID, protocol version, and image format in value32.
 * Pages expose the complete device metadata needed by a generic PC tool.
 * The app generates its own identity header from its ftable.inc. Boot reads
 * the installed app version/build from validated metadata, not a boot constant.
 */
#define CAN_BOOT_CMD_INFO                   (0x07U)
#define CAN_BOOT_INFO_PAGE_CORE             (0x00U)
#define CAN_BOOT_INFO_PAGE_BOOT_VERSION     (0x01U)
#define CAN_BOOT_INFO_PAGE_BOOT_BUILD       (0x02U)
#define CAN_BOOT_INFO_PAGE_APP_VERSION      (0x03U)
#define CAN_BOOT_INFO_PAGE_APP_BUILD        (0x04U)
#define CAN_BOOT_INFO_PAGE_APP_START        (0x05U)
#define CAN_BOOT_INFO_PAGE_APP_CAPACITY     (0x06U)
#define CAN_BOOT_INFO_PAGE_CODE_FLASH       (0x07U)
#define CAN_BOOT_INFO_PAGE_BOOT_RESERVED    (0x08U)
#define CAN_BOOT_INFO_PAGE_ERASE_BLOCK      (0x09U)
#define CAN_BOOT_INFO_PAGE_CAN_CAPS         (0x0AU)
#define CAN_BOOT_INFO_PAGE_NOMINAL_MASK     (0x0BU)
#define CAN_BOOT_INFO_PAGE_DATA_MASK        (0x0CU)
#define CAN_BOOT_INFO_PAGE_REQUEST_ID       (0x0DU)
#define CAN_BOOT_INFO_PAGE_RESPONSE_ID      (0x0EU)
#define CAN_BOOT_INFO_PAGE_PART_0           (0x0FU)
#define CAN_BOOT_INFO_PAGE_PART_1           (0x10U)
#define CAN_BOOT_INFO_PAGE_PART_2           (0x11U)
#define CAN_BOOT_INFO_PAGE_PART_3           (0x12U)
#define CAN_BOOT_INFO_PAGE_FAMILY_0         (0x13U)
#define CAN_BOOT_INFO_PAGE_FAMILY_1         (0x14U)
#define CAN_BOOT_INFO_PAGE_FAMILY_2         (0x15U)
#define CAN_BOOT_INFO_PAGE_APP_IMAGE_SIZE   (0x16U)
#define CAN_BOOT_INFO_PAGE_MIN_IMAGE_SIZE   (0x17U)
#define CAN_BOOT_INFO_PAGE_UPDATE_POLICY    (0x18U)
#define CAN_BOOT_INFO_PAGE_IRQ_ABI          (0x19U)
#define CAN_BOOT_INFO_PAGE_BLOCK            (0x1AU)
#define CAN_BOOT_BLOCK_INFO                 (((unsigned long)CAN_BOOT_BLOCK_DATA_ID << 16) | CAN_BOOT_BLOCK_BYTES)
#define CAN_BOOT_POLICY_WORD                (0x01012001UL)
#define CAN_BOOT_IRQ_ABI                    (0x31563146UL)
#define CAN_BOOT_CMD_METADATA               (0x08U)
#define CAN_BOOT_CMD_SEAL                   (0x09U)
#define CAN_BOOT_CMD_METADATA_RESET         (0x0AU)
#define CAN_BOOT_STATE_APPLICATION          (0x01U)
#define CAN_BOOT_STATE_BOOTLOADER           (0x02U)
#define CAN_BOOT_FIRMWARE_VERSION            (0x00010500UL)
#define CAN_BOOT_FIRMWARE_BUILD              (20261015UL)

/* Bounds polling if the CAN peripheral or bus cannot complete a transition. */
#define CAN_BOOT_CAN_INIT_POLL_LIMIT         (10000UL)
#define CAN_BOOT_CAN_TX_POLL_LIMIT           (10000UL)

#define CAN_BOOT_MANIFEST_MAGIC             (0x334E4143UL) /* "CAN3" */
#define CAN_BOOT_MANIFEST_SIZE              (128U)
/* Reserved Flash extent, independent of the128-byte core and64-byte blocks. */
#define CAN_BOOT_METADATA_REGION_SIZE       (256U)
#define CAN_BOOT_MANIFEST_FLAGS             (0x0003U)
#define CAN_BOOT_MANIFEST_FORMAT_WORD       \
    ((unsigned long)CAN_BOOT_IMAGE_FORMAT_VERSION | \
     ((unsigned long)CAN_BOOT_MANIFEST_SIZE << 8) | \
     ((unsigned long)CAN_BOOT_MANIFEST_FLAGS << 16))
#define CAN_BOOT_MANIFEST_PROTOCOL_WORD     \
    ((unsigned long)CAN_BOOT_PROTOCOL_ID | \
     ((unsigned long)CAN_BOOT_PROTOCOL_VERSION << 16) | \
     ((unsigned long)CAN_BOOT_CHECKSUM_CRC32 << 24))
#define CAN_BOOT_MANIFEST_TARGET_WORD       \
    ((unsigned long)CAN_BOOT_TARGET_ID | \
     ((unsigned long)CAN_BOOT_CAN_MODE_CLASSICAL << 16) | \
     ((unsigned long)CAN_BOOT_CAN_ID_STANDARD << 24))

#define CAN_BOOT_MANIFEST_FORMAT_OFFSET      (0x04UL)
#define CAN_BOOT_MANIFEST_PROTOCOL_OFFSET    (0x08UL)
#define CAN_BOOT_MANIFEST_TARGET_OFFSET      (0x0CUL)
#define CAN_BOOT_MANIFEST_VERSION_OFFSET     (0x10UL)
#define CAN_BOOT_MANIFEST_BUILD_OFFSET       (0x14UL)
#define CAN_BOOT_MANIFEST_SIZE_OFFSET        (0x18UL)
#define CAN_BOOT_MANIFEST_APP_START_OFFSET   (0x1CUL)
#define CAN_BOOT_MANIFEST_CAPACITY_OFFSET    (0x20UL)
#define CAN_BOOT_MANIFEST_CODE_FLASH_OFFSET  (0x24UL)
#define CAN_BOOT_MANIFEST_BOOT_SIZE_OFFSET   (0x28UL)
#define CAN_BOOT_MANIFEST_ERASE_SIZE_OFFSET  (0x2CUL)
#define CAN_BOOT_MANIFEST_NOMINAL_OFFSET     (0x30UL)
#define CAN_BOOT_MANIFEST_DATA_OFFSET        (0x34UL)
#define CAN_BOOT_MANIFEST_REQUEST_ID_OFFSET  (0x38UL)
#define CAN_BOOT_MANIFEST_RESPONSE_ID_OFFSET (0x3CUL)
#define CAN_BOOT_MANIFEST_PART_OFFSET        (0x40UL)
#define CAN_BOOT_MANIFEST_FAMILY_OFFSET      (0x50UL)
#define CAN_BOOT_MANIFEST_MIN_SIZE_OFFSET    (0x5CUL)

/* Timeout advances from the 1-ms TMRJ0 interrupt while flash is idle. */
#define CAN_BOOT_INACTIVITY_TIMEOUT_MS       (30000U)

/* Receive only standard data frames sent to CAN_BOOT_REQUEST_ID. */
#define CAN_BOOT_RX_RULE_ID_LOW              (CAN_BOOT_REQUEST_ID)
#define CAN_BOOT_RX_RULE_ID_HIGH             (0x0000U)
#define CAN_BOOT_RX_RULE_MASK_LOW            (0x07FFU)
#define CAN_BOOT_RX_RULE_MASK_HIGH           (0xC000U)

#if (CAN_BOOT_REQUEST_ID > 0x07FFU)
#error CAN_BOOT_REQUEST_ID must be an 11-bit standard CAN identifier.
#endif

#if (CAN_BOOT_RESPONSE_ID > 0x07FFU)
#error CAN_BOOT_RESPONSE_ID must be an 11-bit standard CAN identifier.
#endif

#if (CAN_BOOT_BLOCK_BYTES != 64U) || (CAN_BOOT_BLOCK_DATA_ID > 0x7F8U) || ((CAN_BOOT_BLOCK_DATA_ID & 7U) != 0U)
#error CAN v5 requires64-byte blocks and an aligned eight-ID standard window.
#endif
#if ((CAN_BOOT_REQUEST_ID & 0x7F8U) == CAN_BOOT_BLOCK_DATA_ID) || ((CAN_BOOT_RESPONSE_ID & 0x7F8U) == CAN_BOOT_BLOCK_DATA_ID)
#error Block data identifiers must not overlap control or response identifiers.
#endif

#endif
