# RL78/F13 CAN boot loader

CAN self-programming boot loader for the Renesas RL78/F13 R5F10BMG.

Start with the [RL78 Boot/App porting guide (zh-TW)](https://released.github.io/slide_boot_loader_for_RL78/#boot-app-porting)
when integrating your own app or designing a different bootloader workflow.

For App metadata/IRQ editing, follow the
[C-header device workshop](https://released.github.io/slide_boot_loader_for_RL78/#device-metadata-vector-workshop).
App users edit app_image_config.h and app_irq_handlers.h. App-mode generation
owns ftable.inc/identity/vector outputs; it does not rebuild Boot. Clean rebuild
App after C header changes; post-build rejects stale linked metadata/vectors.
Each successful App package also emits DefaultBuild/app_build_report.html:
actual BIN metadata, all byte addresses, CRC/SHA-256 and full App vector table.
See the [browser build summary](https://released.github.io/slide_boot_loader_for_RL78/#device-app-build-report).

## Fixed reusable IRQ contract

Boot owns all61 normal positions04..7C with generated assembly. App-only IRQ
additions change only app_irq_handlers.h and a real RETI ISR; INTTRJ0 no
longer calls an ordinary app callback. Reset remains in boot; physical BRK7E
is not forwarded. Compatible app projects retain the MCU/layout, no-init RAM/
startup, metadata/CRC and INFO/ENTER contracts. App version/build are app-local.
See the English and Traditional Chinese IRQ guides for initial Boot/App installation.
The artifact checker requires ROM/FSL below2000 with at least512 spare bytes.
Build/HEX/model checks do not establish physical IRQ/debug/update acceptance.

Protocol5 requires METADATA_RESET0A,64 ordered METADATA08 chunks and SEAL09
before START. Complete256-byte metadata is verified in RAM before flash access.
The128-byte core includes UPD1 DEVICE_APP/CAN policy, F1V1 IRQ ABI and minimum
boot version. Code starts at2200.

## Status

Boot supplies read-only VERIFY, diagnostics capabilities and specific metadata,
policy, IRQ ABI and partition error statuses. App supplies APP_READY after local
initialization checks; Tool compares live version/build/size after update.
Boot UART debug output defaults off (CAN_BOOT_DEBUG_UART); App UART remains.
The post-build verify_boot_budget.ps1 enforces8KiB Boot and minimum512-byte spare.
See [confirmation, diagnostics and acceptance](https://released.github.io/slide_isp_tool_for_RL78/#update-confirmation)
for operating steps and the hardware test matrix. Software models/builds do not
establish physical high-speed, power-loss or IRQ acceptance.

## Public examples and checkout layout

- [CAN Transfer benchmark](https://github.com/released/RL78_F13_CAN_Transfer):
  non-destructive communication testing, not a boot/application image.
- [This Boot project](https://github.com/released/RL78_F13_Boot_loader_CAN).
- [Matching App project](https://github.com/released/RL78_F13_Boot_loader_CAN_app).

Clone Boot and App into the same parent directory using those exact repository
names. Download ZIP users must remove the `-main` directory suffix. App uses
Boot's board sources, CAN configuration and IRQ generator through sibling
references. Build Boot first, then App; initially program `boot_app.hex`, and
use App's `_update.bin` for subsequent CAN updates. A local compiler executable
path in the build command is an installation example, not a source download.

- CS+ / CC-RL build passes with zero errors.
- Target is R5F10BMG: 128 KiB code flash and 8 KiB RAM.
- Boot code and the RL78/F13 FSL stay in `0x0000-0x1FFF`.
- Application update images use the start/capacity declared by metadata and
  store CRC32 in their final four bytes. This F13 build currently advertises
  start `0x2000` and capacity 120 KiB.
- A256-byte format6 metadata region at `0x2100` (128 core/128 reservedFF) identifies MCU, flash layout,
  CAN settings, protocol, version, build, and exact image size.
- CAN transport is polling-only and uses the clock/pin/timing baseline proven
  by `..\RL78_F13_CAN_Transfer` (`pcan.jpg`, `log.jpg`).
- INFO reports boot/application state, target/protocol, firmware version, and
  build without forcing a reset.
- CAN initialization/mode and TX polling are bounded. An invalid app is never
  entered after CAN initialization failure; boot logs and resets to retry.
- One normal physical v5 transfer/END is evidenced above; physical fault and
  power-loss recovery still require board testing.

The compiler uses explicit `-Olite` because the unoptimized image does not fit
in the 8 KiB boot partition. Do not remove it without checking the linker map.

## Hardware and CAN settings

| Item | Setting |
| --- | --- |
| CAN bitrate | 500 kbit/s |
| Clock | X1 4 MHz /2 x16 -> PLL/CPU 32 MHz; CAN=fCLK/2=16 MHz |
| CAN timing | BRP register 1; TSEG1=11, TSEG2=4, SJW=4 TQ; sample 75% |
| CAN format | Classical CAN, standard 11-bit identifiers |
| CTXD0 / CRXD0 | P1.0 / P1.1 |
| Transceiver mode | TJA1050T S=P1.2, LOW for normal TX, HIGH for silent |
| CAN supply | IC2 EVDD=4.75-5.25 V (5 V nominal), not 3.3 V |
| Debug UART0 | 115200 8-N-1, P1.5 / P1.6 |

The boot loader accepts requests on CAN ID `0x700` and responds on `0x701`.
Both the software check and hardware acceptance filter are derived from
`can_boot_config.h`; change the IDs there and rebuild both projects. See
`CAN_BOOT_PROTOCOL.md` for the metadata-driven protocol-v5 frame layout and
master behavior.

`f13_board_support.c/.h` is compiled by both projects. Startup writes CMC in
X1 mode before its write-once lock but stays HOCO64/2 until main, so an absent
crystal cannot hang before the debugger's main breakpoint. Bounded main-loop
initialization selects the benchmark PLL32/MDIV0 clock. UART115200, TMRJ0 and
FSL CPU32 settings are unchanged. A valid inherited PLL is retained on app
handoff. CAN preparation silences TX, masks CAN IRQs, waits for RAM and global/
channel reset, and only then allows driver configuration. Clock failure blocks
CAN and FSL, retaining HOCO32 for diagnostics when fallback succeeds.

Clock status: 0=ready, 1=unexpected/locked setup, 2=X1 timeout, 3=main source,
4=PLL lock, 5=PLL selection, 6=fallback failure. UART startup lists the status
and CAN-ready result; the printed 5-V requirement is not a voltage measurement.
Do not mix this clock baseline with the previous HOCO-only app: initially
program the newly generated paired `boot_app.hex`, or ensure the app region
is erased/invalid when testing boot-only recovery. A previous valid app may
otherwise change the inherited X1/PLL. Boot and App release identities are independent;
minimum Boot, IRQ ABI and layout establish compatibility.

For a non-destructive analyzer test use standard ID `0x700`, DLC2, `07 00`;
expect ID `0x701`, DLC8, INFO page0 with state=bootloader. Do not use the
Custom Transfer `0x710/0x711` frames with this firmware.

Interrupt-vector study and maintenance guides:

- [English guide](INTERRUPT_VECTOR_GUIDE_EN.md)
- [繁體中文指南](INTERRUPT_VECTOR_GUIDE_zh-TW.md)

## Boot decision

The boot loader remains active when any condition is true:

1. RAM address `0xFF700` contains `0xAA55AA55` after an application ENTER.
2. The application reset entry at `0x2000` is erased.
3. The image manifest at `0x2100` is missing or belongs to another target.
4. CRC32 over the manifest-declared image except its final four bytes does not
   match the CRC32 stored in those final four bytes.

Manifest validation also requires format6/core128/flags, protocol/checksum,
target/CAN mode/ID format,256-byte region with128 reservedFF bytes, image516..
122880 aligned4, non-erased version, and exact compiled layout/rates/IDs.
The boot validator also compares ASCII family/part, initialized build, UPD1
policy, IRQ ABI and minimum boot version. CRC integrity is not authentication.
Installed-App version policy and single-use
force operation are described in the [Tool operation guide](https://released.github.io/slide_isp_tool_for_RL78/#force-old-version).

With a valid old app and ENTER,30-second inactivity before accepted START may
return to that app. Matching standard non-RTR ID700/DLC>0 traffic refreshes
the timer before command validation. Invalid-app recovery never enables this
timer, even after START. Accepted-START timeout, ABORT or fatal write/END
errors retain boot and disable automatic return. Rejected START does not
start writing; FSL init failure alone does not disable an existing timer.
Clock/CAN failure resets/retries only for an invalid app; a valid app can run.
Successful END resets and repeats all startup checks before app entry.
The complete gate tables and diagrams are in the
[Boot/App guide](https://released.github.io/slide_boot_loader_for_RL78/#boot-app-porting)
and the [CAN operating guide](https://released.github.io/slide_isp_tool_for_RL78/).

P13.7 is not a boot-entry input. The product requirements assign buttons only
to the F24 host board; the F13 application may still use P13.7/INTP0 for its
own application behavior.

Otherwise it writes the one-shot boot handoff signature at `0xFF714`, sets the
RAM application-owner marker at `0xFF710` to `0x55`, and jumps through the
relocated application table at `0x2000`. After a successful CAN programming
operation it also writes the one-shot update-complete signature at `0xFF718`;
the app consumes and clears both signatures during startup.

## Build

```powershell
& 'C:\Program Files (x86)\Renesas Electronics\CS+\CC\CubeSuite+.exe' `
  /bb '.\RL78_F13_Boot_loader_CAN.mtpj'
```

Important outputs:

| File | Purpose |
| --- | --- |
| `DefaultBuild\boot0000_1FFF.hex` | Boot-only programming image |
| `DefaultBuild\RL78_F13_Boot_loader_CAN.map` | Section-boundary verification |
| `DefaultBuild\RL78_F13_Boot_loader_CAN.abs` | Debug/load module |

Git distribution retains only `DefaultBuild/boot0000_1FFF.hex`; map/ABS and
other compiler outputs are regenerated by a build. The Tool's
[publication workflow](https://github.com/released/simple_renesas_isp_tool/blob/main/docs/GITHUB_PUBLISH_zh-TW.md)
archives other DefaultBuild outputs locally before commit/push.

For E2 Lite debugging, the project runs to `_boot_main`, not `_main`. The link
also uses `-OCDTR`, and normal RAM sections start at `0xFE500` so they do not
overlap the RL78 trace/self RAM area at `0xFE300-0xFE4FF`. See
the interrupt-vector guide's debug section before testing boot-to-app handoff.

Build `..\RL78_F13_Boot_loader_CAN_app` afterward. Send its generated
`DefaultBuild\RL78_F13_Boot_loader_CAN_app_update.bin`; do not send raw HEX.

## Recovery and limitations

- CAN Upgrade v5 requires the 128-byte manifest at image offset `0x100` (device
  address `0x2100` in this build). A legacy image without it is intentionally
  treated as invalid and remains in boot recovery.
- Every written 1 KiB app block is erased before its first 4-byte write. Blocks
  beyond a shorter image are outside its manifest and CRC and are never used by
  the boot handoff.
- The master must wait for an ACK before sending the next DATA frame.
- Repeating the last committed byte offset returns ACK without rewriting flash.
- An interrupted update has an invalid CRC, so reset returns to boot mode.
- If a valid app requested boot but START never arrives, the boot loader returns
  to that app after 30 seconds. Once START is accepted, inactivity resets only
  the transfer session and the target remains in boot mode.
- There is no second bank, rollback, signature verification or encryption.
- FSL uses internal automatic status checking; do not enable user-mode BUSY
  polling or abort an active flash operation.
- Boot and app both use option byte 0 value `0xE9` (`WDTON=0`) and leave the
  WDT module unused. WDT is not part of the product design. Recovery uses
  bounded timeouts, boot-mode retention, manifest/CRC validation, and software
  reset.

The accepted first-release security boundary is CRC32 integrity only;
authentication, signatures and encryption are not provided. Version policy is
an operational guard with an explicit single-update maintenance override.
Product-specific power-failure and FSL fault validation remain required.

## Main files

| File | Responsibility |
| --- | --- |
| `r_main.c` | Boot decision, hardware setup and app handoff |
| `can_boot_transport.c`, `.h` | CAN firmware-update protocol |
| `can_boot_config.h` | Authoritative CAN IDs, target identity and timeout |
| `RSCANconf\` | Renesas RS-CAN Lite driver and 500 kbit/s configuration |
| `fsl_flash_interface.c`, `.h` | RL78/F13 FSL erase/write wrapper |
| `sw_crc32.c`, `.h` | Application CRC32 verification |
| `user_def.h` | Flash layout and board constants |
