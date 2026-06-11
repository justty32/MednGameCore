# actions.json schema 演進

> 日期：2026-06-11
> 定位：ActionDef/ActionLibrary 的 schema 版本化、驗證與錯誤回報、原語擴充路線，以及「原語組合 vs 表達式/腳本」的取捨分析。

依賴：`src/core/turn/action_def.h`/`.cpp`、`data/actions.json`、
`src/core/turn/zone_effects.cpp`（resolve_move/resolve_nova 原語）、
`src/core/turn/action.h`（ActionKind::Skill）。

---

## 1. 現況與痛點

- `ActionLibrary::load_json()` 失敗時 `catch(...) return false` → 呼叫端走
  `load_defaults()`。**typo、缺欄位、型別錯都靜默退回硬編技能**，
  使用者以為改了 JSON 沒生效，其實是載入失敗。
- cereal `CEREAL_NVP` 對缺欄位會丟例外（嚴格），對多餘欄位則忽略——
  意味著 JSON 每筆都得寫滿 11 個欄位（現況 actions.json 確實如此），增欄位即破壞舊檔。
- 沒有 schema 版本，未來欄位改名/改語意無從分辨新舊檔。
- 沒有語意驗證：`weight=0`、`radius=-1`、重複 name 都會默默接受。

## 2. schema_version 欄位（優先序：高）

頂層加版本，與 actions 並列：

```json
{ "schema_version": 2, "actions": [ ... ] }
```

- 載入流程：先讀 `schema_version`（缺欄位視為 1，容忍現有檔）→ 按版本分派解析。
- 規約：**加欄位（有預設值）不升版；改名/改語意/刪欄位才升版**。
- 配套：ActionDef 的 `serialize()` 改用 cereal 的 `make_optional_nvp` 風格——
  cereal JSON 原生沒有 optional NVP，務實做法是自寫小工具
  `load_or(ar, "key", value, default)`（try-catch 包單欄位）或乾脆
  換手寫 JSON 解析（見 §6）。**目標：JSON 只需寫非預設欄位**，
  actions.json 從每筆 11 欄縮回 3-5 欄，可讀性大增。

## 3. 驗證與錯誤訊息品質（優先序：高）

分兩層：

1. **解析層**：失敗時回報「哪個檔、哪筆 action（index 或已讀到的 name）、哪個欄位」。
   `load_json` 簽名改為回傳 `LoadResult { bool ok; std::vector<std::string> errors; }`，
   呼叫端（`zone_world_gd.cpp`）把 errors 餵進既有 trace/debug log 管線
   （`on_trace` → UtilityFunctions::print），開發者在 Godot console 直接看到。
2. **語意層**（解析成功後逐筆檢查）：
   - `weight >= 1`、`radius >= 1`、`dot_kind ∈ {0,1,2}`、各數值非負
   - name 非空、不重複（reindex 時順手檢查）
   - 至少一個原語非零（全零的 action 是資料錯誤）
   - 違規 → 該筆剔除 + error 訊息，**其餘筆照常載入**（部分成功優於全檔退回）

後備規則改為：解析層全敗才 `load_defaults()`，且必吐 warning（不變原則 3）。

## 4. 原語擴充：投射物、召喚、位移（優先序：中）

維持「欄位＝原語開關＋參數」的既有模式，逐個加：

| 原語 | 新欄位草案 | C++ 端工作 |
|---|---|---|
| 位移（dash/knockback） | `displace`（格數，正=自己衝刺，負=擊退命中者） | 新 `resolve_displace`：沿方向逐格檢查 walkable，撞牆截斷；複用 move_dir 編碼 |
| 投射物 | `projectile_range`、`projectile_damage` | 新 `resolve_projectile`：沿方向 raycast 首個 actor/牆；回合制下「即時命中的直線 nova」即可，不需要飛行實體 |
| 召喚 | `summon_proto`（字串，引用 prototype id）、`summon_count` | **依賴 02 的 PrototypeDef**——召喚是「prototype 資料化」的第一個跨檔引用，建議在 02 落地後做 |

風險控制：欄位超過 ~15 個時可讀性開始崩，屆時把 ActionDef 重構成
`effects: [ { "kind": "nova", ... }, { "kind": "dot", ... } ]` 陣列形式
（一個 action 多個效果步驟）——這是 schema_version 升 2→3 的天然時機。

## 5. 表達式/腳本 vs 原語組合：取捨分析（優先序：低，傾向不做）

| 面向 | 原語組合（現況路線） | 表達式（如 `damage = 2*level+3`） | 腳本（Lua/quickjs） |
|---|---|---|---|
| 可測試性 | ctest 直接測原語，極佳 | 要測表達式引擎本身 | 要測 binding 層，最差 |
| 存檔相容 | def 不入存檔，無影響 | 同左 | 腳本狀態難序列化 |
| modder 上限 | 只能組合既有原語 | 數值公式自由 | 行為自由 |
| 失敗模式 | 載入期即驗證 | 執行期才爆 | 執行期爆 + 沙箱問題 |
| 依賴 | 零 | 小型 eval 器（可自寫） | 外部 VM、跨平台建置成本 |

**結論建議**：本專案是「自家內容為主、modding 為遠期」，原語組合的天花板
（搭配 §4 的 effects 陣列）足以涵蓋 roguelike 技能 9 成需求；
**腳本不做**。表達式可考慮一個極窄的折衷：允許數值欄位寫
`"damage": "2*floor+3"` 字串、載入期編譯成係數表（只支援 `a*var+b` 線性式），
給「隨樓層成長的怪物技能」用——這在 02/03 樓層縮放需求出現時再評估。

## 6. cereal 之外的選項（備忘）

cereal JSON 適合「C++ 結構鏡像」，不適合「寬鬆 schema + 好錯誤訊息」。
若 §2/§3 在 cereal 上做得太彆扭，換 nlohmann/json（header-only，FetchContent 一行）
手寫 `from_json(const json&, ActionDef&, LoadResult&)` 是務實退路——
**只換 data 檔解析，存檔仍走 cereal binary**，兩者本來就是不同管線
（`src/core/serialize/` 不受影響）。
