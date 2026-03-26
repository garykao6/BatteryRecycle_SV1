## 1. 行為確認與佔位文案

- [ ] 1.1 與產品確認「未取得站名」時 `station_lb` 顯示（佔位字或空白）
- [ ] 1.2 確認長站名是否需在 `.ui` 做省略或換行（必要時新增子任務）

## 2. 站名資料流與 UI 同步

- [ ] 2.1 依 `design.md` 實作單一路徑更新（訊號集中於 `MqttHelper`/`global` 或等效處）
- [ ] 2.2 `HomePage`、`mb_putinbatterypage`、`nmb_putinbatterypage`、`mb_endpage`、`nmb_endpage` 之 `station_lb` 訂閱同一更新來源，MQTT 晚到時可刷新目前頁面
- [ ] 2.3 手動驗證：先進投放流程再收到 `storeInfoAck` 時，站名是否正確顯示

## 3. 時間顯示

- [ ] 3.1 確認各頁 `time_lb` 皆連接 `ClockBus::minuteTick` 且格式與 `clockbus.cpp` 一致
- [ ] 3.2 若發現未訂閱或重複訂閱，整理為單一清晰連接（避免洩漏或雙重更新）

## 4. 驗收

- [ ] 4.1 對照 `specs/store-and-time-display/spec.md` 逐條情境走查
- [ ] 4.2 必要時執行 `/opsx:verify`（或專案約定之檢查）後再歸檔變更
