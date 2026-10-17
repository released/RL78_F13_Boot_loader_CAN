/* Shared by the real boot transport and host-side fault tests. C90 only.
 * No erase/write operation is reachable through staging or validation. */
static unsigned long g_metadata_words[64];
static unsigned short g_metadata_staged;
static unsigned char g_metadata_sealed;
static unsigned char g_metadata_error;
/* No armed permission: FORCE_START applies only to its own START request. */
#define VersionAllowed(policy, installed, command) \
    ((command) == CAN_BOOT_CMD_FORCE_START || (policy) != 3U || \
     g_metadata_words[4] >= (installed))

static void MetadataReset(void)
{
    g_metadata_staged = 0U;
    g_metadata_sealed = 0U;
}

static const unsigned long g_manifest_contract[32] = {
    CAN_BOOT_MANIFEST_MAGIC, CAN_BOOT_MANIFEST_FORMAT_WORD,
    CAN_BOOT_MANIFEST_PROTOCOL_WORD, CAN_BOOT_MANIFEST_TARGET_WORD,
    0UL, 0UL, 0UL,
    CAN_BOOT_APPLICATION_START, CAN_BOOT_APPLICATION_CAPACITY,
    CAN_BOOT_CODE_FLASH_SIZE, CAN_BOOT_RESERVED_SIZE, CAN_BOOT_ERASE_BLOCK_SIZE,
    CAN_BOOT_NOMINAL_500K_MASK, CAN_BOOT_DATA_BITRATE_MASK,
    CAN_BOOT_REQUEST_ID, CAN_BOOT_RESPONSE_ID,
    CAN_BOOT_MCU_PART_0, CAN_BOOT_MCU_PART_1, CAN_BOOT_MCU_PART_2, CAN_BOOT_MCU_PART_3,
    CAN_BOOT_MCU_FAMILY_0, CAN_BOOT_MCU_FAMILY_1, CAN_BOOT_MCU_FAMILY_2,
    CAN_BOOT_MINIMUM_IMAGE_SIZE,
    0x31445055UL, CAN_BOOT_POLICY_WORD, CAN_BOOT_IRQ_ABI, 0UL,
    0UL, 0UL, 0UL, 0UL
};

static unsigned char MetadataValid(const unsigned long __far *words)
{
    unsigned short i;
    unsigned long size;
    size = words[6];
    g_metadata_error = CAN_BOOT_STATUS_BAD_IMAGE;
    if (size < CAN_BOOT_MINIMUM_IMAGE_SIZE || size > CAN_BOOT_APPLICATION_CAPACITY ||
        (size & 3UL) || words[4] > 0xFFFFFFUL || words[5] == 0xFFFFFFFFUL || words[29] > 1UL)
    {
        return 0U;
    }
    g_metadata_error = CAN_BOOT_STATUS_POLICY;
    if (words[27] > CAN_BOOT_FIRMWARE_VERSION) return 0U;
    for (i = 0U; i < 64U; i++)
    {
        if ((i >= 4U && i <= 6U) || i == 27U || i == 29U) continue;
        if (words[i] != ((i < 32U) ? g_manifest_contract[i] : 0xFFFFFFFFUL))
        {
            g_metadata_error = (i == 25U) ? CAN_BOOT_STATUS_POLICY : (i == 26U) ? CAN_BOOT_STATUS_IRQ_ABI :
                ((i >= 7U && i <= 11U) ? CAN_BOOT_STATUS_LAYOUT : CAN_BOOT_STATUS_METADATA);
            return 0U;
        }
    }
    return 1U;
}

static unsigned char MetadataStage(unsigned short offset, const unsigned char *data)
{
    unsigned char i;
    unsigned char *bytes;
    bytes = (unsigned char *)g_metadata_words;
    if ((offset & 3U) || offset > 252U || offset > g_metadata_staged) return CAN_BOOT_STATUS_BAD_SEQUENCE;
    if (offset < g_metadata_staged)
    {
        for (i = 0U; i < 4U; i++) if (bytes[offset+i] != data[i]) return CAN_BOOT_STATUS_BAD_IMAGE;
        return CAN_BOOT_STATUS_OK;
    }
    for (i = 0U; i < 4U; i++) bytes[offset+i] = data[i];
    g_metadata_staged += 4U;
    g_metadata_sealed = 0U;
    return CAN_BOOT_STATUS_OK;
}

static unsigned char MetadataSeal(unsigned long expected_crc)
{
    g_metadata_sealed = 0U;
    if (g_metadata_staged != 256U) return CAN_BOOT_STATUS_BAD_IMAGE;
    if (!MetadataValid((const unsigned long __far *)g_metadata_words)) return g_metadata_error;
    if (F13_Crc32Bytes((const unsigned char __far *)g_metadata_words, 256UL) != expected_crc)
        return CAN_BOOT_STATUS_CRC_ERROR;
    g_metadata_sealed = 1U;
    return CAN_BOOT_STATUS_OK;
}
