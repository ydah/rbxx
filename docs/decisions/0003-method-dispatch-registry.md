# ADR 0003: 初版のメソッドディスパッチ

- 背景: callable 型ごとの static trampoline は同型関数ポインタの衝突回避が複雑になる。
- 選択肢: 一意 trampoline + fallback / `(owner, ID)` レジストリ一本。
- 決定: Phase 3 の初版は設計書が許容するレジストリ方式に統一する。
- 決定: 定義は mutex 保護、呼出し開始後のテーブルは read-only とする。
- 影響: 実装と例外境界を単純化し、全 callable 種別を同じ経路で扱える。
- 影響: Phase 9 のベンチが目標未達の場合のみ一意 trampoline を再検討する。

## Phase 9 追記

- 計測結果: 初期レジストリ経路は 0 引数 int メソッドで手書き C 比 18.6 倍となり、G2 を満たさなかった。
- 決定: 汎用レジストリは kwargs / block / overload 用に維持し、単一 0 引数 member は固定 arity slot を使う。
- 決定: `def<&T::method>("name")` は member pointer も trampoline に焼き込み、最短経路を明示できる API とする。
- 互換性: 同名 overload が追加された時点で Ruby method を汎用 trampoline に再定義するため、意味論は変わらない。
- 結果: compile-time 経路は手書き C 比 1.02 倍、Rice 比約 3 倍を記録した。
