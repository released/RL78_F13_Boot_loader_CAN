# MCU CAN device golden sample

Read README.md and the App README before changing metadata/vector tooling.
F13 and F24 CAN device Boot/App are joint golden samples for future MCU ports.
Preserve the common authoring/build/validation workflow in both projects.
Prioritize both MCU device projects before F24 host work; do not modify host
firmware as part of device metadata/vector changes.

- Users edit app_image_config.h and app_irq_handlers.h in each App.
- image_metadata_config.h owns neutral IMAGE_* format/MCU/Flash/policy/Boot
  identity; can_boot_config.h owns CAN only; boot_app_handoff.h owns RAM/owner.
  Keep explicit independent includes and the three-header build readers aligned.
- Compile image_metadata.c/.h independently in Boot/App; the local copies
  match Tool firmware/common/device_metadata via sync_device_metadata.ps1.
  image_metadata_profile.c owns expected values. Parser has no CAN/Flash API;
  transport staging and CRC/launch gates stay separate. Keep C90,32-bit word/
  endian/alignment boundaries, detailed/compact diagnostic acceptance equal.
- app_config.ps1 is a local identical copy of the common device-header reader.
  No JSON fallback or Python build dependency. Keep parser copies synchronized.
- Pre-build generates INC/identity/vectors; post-build rejects stale compiled
  metadata and IRQ tables. App-only changes must not rewrite Boot images.
- app_build_report.ps1 is the identical golden post-package HTML helper.
  Keep its metadata/CRC/hash/vector data tied to the actual validated BIN;
  no Python/report-service dependency or inference of hardware success.
- Preserve full normal IRQ forwarding, AA/55 owner, RETI/RETB distinction,
  256-byte metadata, CRC/range checks and the 8KiB/minimum512-byte spare guard.
- Device image magic is CAN plus NUL (0x004E4143). Policy3 stores the sixth
  App define APP_IMAGE_VERSION_CONTROL at metadata0x74. Only a valid installed
  App supplies the policy/version floor; ordinary START allows same/newer
  when enabled. FORCE_START0E skips only ordering for that transaction, not
  identity/CRC/layout/IRQ/minimum-Boot checks; no armed/persistent override.
- Boot and App version/build are independent. App minimum Boot is produced
  from the paired Boot contract, not an equality requirement on App version.
- Handwritten embedded C90, block-start declarations and English comments.
- Hardware/COM/power tests need a board and explicit scope; report NOT RUN.
- Sync the device MD/web/PDF and engineering verification after changes.
