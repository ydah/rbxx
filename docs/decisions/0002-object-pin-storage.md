# ADR 0002: object の pin ストレージ

- 背景: 設計案の Array スロット解放はデストラクタから Ruby Array API を呼び、raise の危険がある。
- 選択肢: 隠し Array + free list / `rb_gc_register_address` した共有セル。
- 決定: `VALUE` を持つ共有セルを登録し、copy はセルを共有、move は所有権を移す。
- 影響: pin は compaction 時に更新され、最後の handle 解放時は非 raise API だけを呼ぶ。
- 影響: Array のスロット管理は不要になり、observable な copy/move semantics は設計どおりとなる。
