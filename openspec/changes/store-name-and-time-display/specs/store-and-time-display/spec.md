## ADDED Requirements

### Requirement: 站點名稱顯示來源與同步

系統 SHALL 在公開操作相關畫面顯示目前站點名稱，字串 SHALL 與 `global::storeInfo` 一致；該值於收到 MQTT `storeInfoAck` 且 payload 內 `store.name` 非空時更新。

#### Scenario: 成功取得站名

- **WHEN** 後端經 MQTT 回傳有效之 `store.name`
- **THEN** 應用程式 SHALL 將 `global::storeInfo` 設為該字串，並 SHALL 更新所有已實作站名標籤之畫面上對應 `QLabel` 文字

#### Scenario: 尚未取得站名

- **WHEN** 應用程式已顯示相關畫面但尚未收到有效 `store.name`
- **THEN** 系統 SHALL 依設計顯示佔位文字或空白（與 `design.md` 一致），且 SHALL 不得在收到有效站名後仍保留錯誤舊值

### Requirement: 時間顯示格式與更新

系統 SHALL 在首頁及投放／結束流程等已連接 `ClockBus` 之畫面，以繁體中文（台灣）地區慣見格式顯示目前本地日期時間；更新頻率 SHALL 與 `ClockBus` 公開訊號一致（目前為對齊每分鐘之 `minuteTick`）。

#### Scenario: 分鐘邊界更新

- **WHEN** `ClockBus` 發出 `minuteTick` 並帶有格式化時間字串
- **THEN** 所有已訂閱該訊號之畫面時間標籤 SHALL 更新為該字串

#### Scenario: 地區設定

- **WHEN** 顯示時間字串
- **THEN** 格式 SHALL 使用專案設定之台灣中文地區（`QLocale::Chinese, QLocale::Taiwan`）產生，與 `ClockBus` 實作一致

### Requirement: 跨頁一致性

若畫面包含站點名稱與時間兩者，兩者 SHALL 遵循上述同一資料來源與同一時鐘機制，避免僅單一頁面更新。

#### Scenario: 進入投放流程

- **WHEN** 使用者由首頁進入會員或非會員投放頁
- **THEN** 該頁之站點標籤 SHALL 反映當下 `global::storeInfo`，時間標籤 SHALL 持續依 `ClockBus` 更新
