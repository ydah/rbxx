# ADR 0003: 初版のメソッドディスパッチ

- 背景: callable 型ごとの static trampoline は同型関数ポインタの衝突回避が複雑になる。
- 選択肢: 一意 trampoline + fallback / `(owner, ID)` レジストリ一本。
- 決定: Phase 3 の初版は設計書が許容するレジストリ方式に統一する。
- 決定: 定義は mutex 保護、呼出し開始後のテーブルは read-only とする。
- 影響: 実装と例外境界を単純化し、全 callable 種別を同じ経路で扱える。
- 影響: Phase 9 のベンチが目標未達の場合のみ一意 trampoline を再検討する。
