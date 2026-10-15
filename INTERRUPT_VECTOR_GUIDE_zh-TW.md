# F13 固定完整中斷介面：image format6

本版取代舊的 direct-VECTN／共用 C callback 做法。第一次須一起安裝
boot1.4.0 與 format6 app；之後在固定契約內新增一般 app 中斷，只更新 app，
不需重燒 boot。不是任意 standalone 專案都能不經整合直接套用。

第一次移植先讀 [RL78 Boot/App 工程移植入口](https://released.github.io/slide_boot_loader_for_RL78/#boot-app-porting)，
本篇補充 F13 向量實作細節，不取代記憶體、startup、封裝與實板驗證。

## 固定契約

- MCU：R5F10BMG。Boot0000..1FFF；app2000..1FFFF。
- App2000..20FF：64個4-byte slot，位址＝2000＋實體 vector address×2。
- App2100..21FF：256-byte metadata，128有效 core／128 reservedFF；code2200起。
- App 長度由 map 決定，4-byte aligned，最小516、最大122880 bytes。
  CRC32 放在實際映像最後4 bytes，不固定在1FFFC。
- FF700 recovery request、FF710..FF71F no-init／owner／handoff 不得被 app
  startup 或 stack 清除；資料與 debug trace RAM 也不可重疊。
- 保留4MHz X1／PLL32／CPU32／CAN16 的 shared board support 與交接，
  不可在 app 啟動時停止已有效的 inherited PLL。
- Format6、CAN protocol4、targetF13B、standard ID700／701、
  metadata／geometry／CRC 驗證仍必須一致。

App 版本／build 以自己 ftable.inc 為來源，pre-build 產生 app_identity.h。
Boot 從已驗證的 installed metadata 讀 app 版本，不綁定 app 編譯版本。

## 向量歸屬與返回

Reset 仍屬 boot cstart。Boot 固定持有04..7C 的61個一般位置，包含保留位置；
app 不能啟用保留 vector。02保留，physical BRK7E 不轉送，留給 debugger／
device 設定。Linker 目前填入0000，這不是 app BRK API，正式 app 不可執行 BRK。

FF710＝AA：INTTRJ0 進 boot timer ISR，其餘進 boot 預設 RETI。
FF710＝55：轉送 app 對應 slot。其他 owner 值只進 boot 預設，不跳 app。

全部一般 app ISR，包含 r_tmr_rj0_interrupt，都必須是非 static 的正常 ISR：

```c
#pragma interrupt your_handler
void __near your_handler(void)
{
    /* Acknowledge the source and capture bounded state only. */
}
```

不可加 vect＝，不可把 ISR 當普通 C 函式呼叫；以 RETI 返回。
Boot／app 是獨立連結的專案，可以使用相同函式名，不必為相同 vector 改名。
需要避免的是同一實體向量有重複 owner，不是兩個專案用了相同 symbol 名稱。

F13 為維持8KiB boot，以4-byte stub（near CALL＋vector literal）搭配共用組語
分流。它保存 AX／HL／ES，以 ES＝0 讀 vector literal，計算 app slot，
只改寫自己額外壓入的 CALL 返回位址，恢復暫存器並 RET 到選定 ISR／slot。
額外暫時 stack 是10 bytes（CALL4＋保存6），原硬體 PC20／PSW frame 不變；
最終 ISR 的 RETI 消耗原 frame。這個內部 RET 不是舊的 app callback 特例。
分流期間不重新 EI、不切 register bank。

F24 對 app 的原則相同，但用逐向量直接分支。兩者 MCU 的暫存器、flash library
與分區仍不同，不能直接混用 firmware。

## 新增 app IRQ／整合不同 app 專案

1. 保留 app cstart、no-init／stack、ftable.inc、manifest／CRC post-build、
   shared board support、INFO／ENTER／reset request。不要沿用 standalone
   專案下載低位址 vector 的方式部署 app。
2. 新增 ISR 來源並加入 app MTPJ。採上述無 vect 的 interrupt pragma；
   不能 static、不能回普通 RET。Code Generator 重新產生後必須再核對。
3. 在 app_irq_handlers.json 加入實體 vector 與函式名，例如
   "0x34": "your_adc_handler"；不是中斷序號，只接受04..62有效一般位置。
4. App pre-build 執行 generate_irq_contract.ps1 -Mode App -AppProject "."，
   檢查已登錄來源的 pragma，產生 app_irq_table.inc／app_identity.h。
   不要手改生成檔，也不要修改 boot 的向量設定。
5. 在 EI 前設定 app owner55、初始化周邊、清 pending flag、設定優先權並解除遮罩。
   新 app 必須保留 FF700／FF710 no-init；不能被 runtime stack 初始化掃掉。
6. 相容變更只 build／更新 app。檢查 table、metadata、BIN 實際長度與 CRC，
   只部署從2000開始的 update HEX／BIN；不得部署 app ABS 的暫時低位址 .vect。
7. 實測新 IRQ 與既有 timer／UART、INFO／ENTER、CAN update／reset。
   若 app 未實作 INFO／ENTER，Tool 就無法要求執行中的 app 自動回 boot。

未用一般 slot 轉到預設 RETI，不再是FFFFFFFF。BRK 保留 slot 是 RETB，
但 physical BRK 不轉送。預設 RETI 不替你清周邊原因，未使用的 IRQ 必須遮罩，
否則可能產生 interrupt storm。CRC 正確不代表 ISR 語意正確。

改 MCU、分區、共用 RAM、startup 假設、boot 本身使用的 IRQ、
更新通訊或 metadata ABI，仍可能需要改 boot，不屬於一般 app-only 擴充。

## 檢查與除錯

Boot pre-build 與 app mapping 無關；不可新增競爭的 -VECTN 或 C vect pragma。
Tool 專案 scripts/verify_f13_artifacts.ps1 檢查實際 HEX 的全部向量、
app branch／RETI／RETB、RAM／ROM／映像範圍、metadata 與 CRC。
tests/test_f13_irq_contract.py 以實際 dispatcher HEX 做 stack／register 模型，
並驗證保留／錯誤 mapping 拒絕與 app-only mapping；不是硬體模擬器或板測。

Boot 至少保留512 bytes。現行 format6 產物 boot/FSL end1DD1，剩558 bytes；
每次編譯須以實際 map 與 artifact checker 重驗，不能固定沿用此數字。
F13 dispatcher 組語299 bytes；F24逐向量部分854 bytes，這是本次產物的數字，
不是所有 MCU／編譯版本都固定相同。

CS+ boot Run-to-symbol＝boot_main（_boot_main），standalone app＝main（_main）。
保留 -OCDTR、FE300..FE4FF trace RAM；normal data 從FE500以上開始。
Standalone app debug 會暫時改寫低位址 vector，不代表正式 handoff 通過。
整合測試須保留 boot vector，只載入 app symbol，reset 經 boot_main 再停 app main。

待實板驗證：初次 matching install、boot／app timer、新 IRQ 只更新 app、
E2 lite halt／continue／stop、INFO／ENTER update、傳輸中 reset／斷電、
錯誤 CRC recovery、brownout。現行專案不使用 WDT；產品若啟用須另驗。
建置與模型通過不能代替以上板測。
