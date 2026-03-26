## Why

首頁與投放／結束流程等多處需顯示**站點名稱**與**目前時間**，資料來源分別為 MQTT 回傳的 `storeInfo` 與 `ClockBus`；若格式不一致、未載入時留白、或僅分鐘更新，會影響現場辨識與除錯。本變更將把顯示行為收斂為清楚、可預期的規則。

## What Changes

- 定義站點名稱在 UI 上的**顯示時機**（例如 MQTT 未到前之佔位字或隱藏策略）與**字串來源**（`global::storeInfo`）。
- 定義時間顯示的**格式**（地區／語系）、**更新頻率**（維持分鐘對齊或改為秒級，視設計而定）與**適用頁面**（首頁、會員／非會員投放頁、結束頁等）。
- 確保相關 `QLabel`（如 `station_lb`、`time_lb`）與 `ClockBus::minuteTick`／`storeInfoAck` 流程一致，避免僅部分頁面更新。

## Capabilities

### New Capabilities

- `store-and-time-display`: 站點名稱與本地時間在公開畫面之一致顯示規則（資料流、佔位、格式與更新）。

### Modified Capabilities

- （無）— `openspec/specs/` 目前尚無既有能力規格；本次以新增 delta 規格為主。

## Impact

- **程式**：`homepage.cpp`／`homepage.ui`、`mb_putinbatterypage.*`、`nmb_putinbatterypage.*`、`mb_endpage.*`、`nmb_endpage.*`、`clockbus.cpp`、`global.*`、`mqtthelper.cpp`（`storeInfo`／`storeInfoAck`）。
- **系統**：仍依後端 MQTT `storeInfoAck` 提供站名；時間仍依裝置本地時區與 `QLocale::Taiwan`。
