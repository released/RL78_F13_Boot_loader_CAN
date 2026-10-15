# F13 fixed interrupt interface, image format6

This replaces the legacy direct-VECTN / shared C callback design. Boot1.4.0
and format6 app images must initially be installed together. Normal app IRQ
additions then require only an app update, not a new boot image.

For the complete engineering workflow, start with the
[Boot/App porting guide (zh-TW)](../../../python/simple_renesas_isp_tool/docs/RL78_BOOT_APP_PORTING_GUIDE_zh-TW.md).

## Stable contract

- MCU R5F10BMG; boot0000..1FFF, app2000..1FFFF.
- App2000..20FF:64 four-byte slots; slot=2000+physical-vector-address*2.
- App2100..21FF:256-byte metadata (128 core,128 erasedFF); code starts2200.
- Image length is map-derived, aligned to4, minimum516, maximum122880.
  CRC32 occupies the final four bytes of the actual image, not necessarily1FFFC.
- FF700 recovery request and FF710..FF71F no-init state must survive app
  startup. Keep stack, normal data and debug trace RAM outside these regions.
- Retain the shared X1=4MHz / PLL32 / CPU32 / CAN16 handoff support. Do not
  stop a valid inherited PLL in app startup.
- Metadata format6, CAN protocol4, targetF13B, standard IDs700/701 and exact
  metadata/geometry/CRC validation remain mandatory.

The boot reads installed app version/build from validated metadata. The app's
own ftable.inc generates app_identity.h during pre-build; changing app version
does not change a boot constant. An arbitrary standalone project is NOT
automatically compatible merely because it has the same MCU name.

## Ownership and returns

Reset belongs to boot cstart. All61 normal positions04..7C have fixed boot
assembly stubs, including reserved positions which must never be enabled.
Reserved02 and physical BRK7E are not forwarded. The linker currently fills
the unowned BRK slot with0000; it is not an app BRK API. Keep debugger/device
ownership and do not issue BRK from production app code.

FF710=AA selects boot: INTTRJ0 tail-dispatches to f13_boot_timer_isr; other
positions select boot default RETI. FF710=55 selects the matching app slot.
Any other owner value selects boot default RETI, never an app slot.

Every normal app handler, including r_tmr_rj0_interrupt, is a global,
non-static ISR with a normal pragma and no physical vect assignment:

```c
#pragma interrupt your_handler
void __near your_handler(void)
{
    /* Acknowledge the source and capture bounded state only. */
}
```

The handler returns RETI. Never call an ISR as an ordinary C function.
Identical function names in separately linked boot/app projects are legal;
only duplicate physical vector ownership is forbidden. Each side's handler
body is independent and need not share the other side's symbol name.

F13 uses4-byte stubs (near CALL plus vector literal) and one shared assembly
dispatcher to fit8KiB boot. The dispatcher saves AX/HL/ES, reads the literal
with ES=0, computes the app slot, rewrites ONLY its synthetic CALL return PC,
restores registers and RETs to the selected ISR/slot. CALL frame4 + saved
registers6 =10 temporary stack bytes. The original hardware PC20/PSW frame is
untouched. The final ISR RETI consumes that original frame. No early EI or
register-bank switch occurs. This internal RET is not the removed app callback.

F24 has the same app-facing rule but uses direct per-vector branches.
Different MCU register/flash/partition implementations are not interchangeable.

## Adding an app IRQ or integrating another app project

1. Copy the app integration structure, not the boot linker options. Retain
   cstart handoff, no-init RAM/stack layout, ftable.inc, manifest/CRC post-build,
   shared board support and app-side INFO/ENTER/reset-request handling.
2. Add the new source to the app MTPJ. Define the global ISR above. Preserve
   it in Code Generator user areas; no vect=, static or ordinary RET handler.
3. Add the physical vector-to-symbol mapping to app_irq_handlers.json.
   Example: "0x34": "your_adc_handler". Use the MCU's vector address, not an
   ordinal. The generator accepts real normal positions04..62 only.
4. App pre-build runs generate_irq_contract.ps1 -Mode App -AppProject ".",
   validates registered normal interrupt pragmas, and generates app_irq_table.inc
   and app_identity.h. Do not hand-edit generated files.
5. Initialize the peripheral, clear pending flags and set priority before
   unmasking it. Set app owner55 before EI; preserve no-init state in startup.
6. Build ONLY app for a compatible change. Verify HEX bounds, table branches,
   metadata, actual size and CRC. Deploy only the update BIN/HEX beginning2000.
   Do not deploy the app ABS's temporary low-address reset/vector section.
7. Test the new IRQ, existing timer/UART IRQs, INFO/ENTER, CAN update and reset.
   If app omits INFO/ENTER, Tool cannot request a running app to re-enter boot.

All unused normal slots branch to default RETI, rather than erasedFFFFFFFF.
The reserved BRK app slot points to RETB, but physical BRK is not forwarded.
Default RETI does not clear peripheral causes: keep unused IRQs masked or an
interrupt storm can occur. A valid CRC does not prove ISR semantics.

Changing boot-used IRQs, MCU, partition, shared RAM, startup assumptions or
metadata/protocol ABI is not an app-only extension and can require new boot.

## Build and verification

CS+ pre-build generates boot assembly independently of app_irq_handlers.json.
No -VECTN options or C vect pragmas may compete with it. The tool project's
scripts/verify_f13_artifacts.ps1 checks generated files, actual physical
vectors, app branches/RETI/RETB, RAM/ROM/image bounds, metadata and CRC. Its
tests/test_f13_irq_contract.py executes the actual dispatcher HEX through a
small deterministic stack/register model, and tests invalid/app-only mappings.
This model is not a hardware simulator or physical timing verification.

Boot/FSL must retain at least512 bytes before2000. Current format6 build ends
at1DD1, with558 bytes free; recheck the actual map for every build.
Dispatcher assembly is299 bytes versus F24's854
per-vector bytes; these are current artifacts, not cross-MCU size guarantees.

## Debug and hardware acceptance

Boot Run-to-symbol is boot_main (_boot_main); standalone app is main (_main).
Retain -OCDTR and reserve FE300..FE4FF; normal data starts at or aboveFE500.
Standalone app debug may temporarily replace physical vectors via its ABS.
For production handoff tests retain boot vectors, load app symbols only,
reset through boot_main and break at app main. Do not download app low vectors.

Required MANUAL tests: initial paired install, boot and app timer, app-only
IRQ added without boot reflash, E2 lite halt/continue/stop, INFO/ENTER update,
reset/update interruption, invalid CRC recovery and brownout. WDT is unused
in this reference; enabling it requires additional product validation. None is
established by successful builds or the model test alone.
