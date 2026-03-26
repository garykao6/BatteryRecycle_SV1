## Context

- **站點名稱**：`MqttHelper::sendstoreInfo()` 請求後，由 `storeInfoAckReceived` 解析 `store.name` 寫入 `global::storeInfo`；`HomePage::onstoreInfoAckReceived` 已更新 `ui->station_lb`。`mb_putinbatterypage`、`nmb_putinbatterypage`、`mb_endpage`、`nmb_endpage` 於建構時將 `station_lb` 設為 `global::storeInfo`，但若 MQTT 晚於頁面建立，可能未自動刷新。
- **時間**：`ClockBus` 以 `QLocale(Chinese, Taiwan)` 對齊每分鐘發出 `minuteTick`；首頁與上述投放／結束頁已連接並設定 `time_lb`。

## Goals / Non-Goals

**Goals:**

- 站名在所有含 `station_lb` 的公開畫面與 `global::storeInfo` 一致；MQTT 晚到時能補更新。
- 時間顯示格式與 `ClockBus` 一致；各已訂閱頁面行為一致。
- 實作變更集中、可測（例如單一槽函式或小型 helper，避免複製貼上發散）。

**Non-Goals:**

- 變更後端 MQTT 契約或新增欄位。
- 改為秒級時鐘（除非後續獨立變更擴充 `ClockBus`）。
- 重構整個導覽或 `QStackedWidget` 架構。

## Decisions

1. **站名更新策略（相對於僅在 HomePage 更新）**  
   - **決定**：在 `global::storeInfo` 被寫入的單一路徑（例如 `MqttHelper::storeInfoAckReceived` 發射後、或集中處理函式）觸發 UI 更新，或發射 Qt 訊號（如 `storeInfoChanged(QString)`）讓各頁訂閱。  
   - **理由**：避免每頁各自輪詢；MQTT 晚到時所有可見頁同步。  
   - **替代方案**：僅在 `HomePage` 更新——已證實不足以覆蓋已切換之頁面。

2. **未取得站名時的顯示**  
   - **決定**：採用固定佔位字串（例如「—」或「載入中…」）或維持空白，須與 UI/產品一致；預設建議可見佔位以利現場辨識。  
   - **理由**：空白難以區分故障與未載入。

3. **時間**  
   - **決定**：維持 `ClockBus::minuteTick`；若需秒級，另開變更評估 timer 負載與對齊策略。  
   - **理由**：符合現有實作與規格；降低本次範圍。

## Risks / Trade-offs

- **[風險] 訊號循環或重複連接** → **緩解**：單例或 `MqttHelper` 單一 `storeInfoChanged`；頁面用 `Qt::UniqueConnection` 或於 `showEvent` 單次綁定（依現有架構選擇較少侵入者）。
- **[風險] 字串過長撐破版面** → **緩解**：`QLabel` 設 `elide` 或 `wordWrap`／`maximumWidth`（若產品同意）。

## Migration Plan

- 部署為一般應用程式更新；無資料庫遷移。若佔位字變更，僅影響畫面文案。
- 回滾：還原至先前 commit；站名可能回到「僅首頁即時更新」行為。

## Open Questions

- 未取得站名時的精確佔位文案（需產品/現場確認）。
- 長站名是否需在 `.ui` 層級限制寬度或省略號。
