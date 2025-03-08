# ImageLoader_M
AviUtlで最大画像サイズを超える画像のロードを行うスクリプト

## 概要
抹茶鯖のAodaruma氏のプロトタイプをもとに作ってみたものを、ePi氏に添削していただき、機能追加を行ったものです。  
内部で4GB制限にかからないような画像のキャッシュを独自に行っています。

また、黒猫大福氏の[ごちゃまぜドロップスで最大画像サイズを超える画像をD&Dで読み込めるようにするやつ](https://roku14live.booth.pm/items/4569547)に以下の変更を行ったものを同梱させていただいております。
1. `exedit.ini`で画像ファイルとして指定した拡張子も Shift+D&D で読み込めるように
2. `P.name`を`ImageLoader_M Dropper`に変更
3. オリジナル版のライセンス表記をファイル内に記載

## 導入方法
1. `exedit.auf`と同一ディレクトリにある`script`ディレクトリ内、またはそのディレクトリの1階層下に`ImageLoader_M.obj`,`ImageLoader_M.dll`を配置
2. (任意)`GCMZDrops`ディレクトリを`exedit.auf`が存在するディレクトリに D&D

## 読み込み可能な画像形式
+ GDI+ で読み込み可能な画像形式
+ exedit.auf と同一ディレクトリ内の Susie-Plugin で読み込み可能な画像形式

GDI+で読み込み可能な画像形式については[こちら](https://docs.microsoft.com/ja-jp/windows/win32/gdiplus/-gdiplus-types-of-bitmaps-about#graphics-file-formats)などを参照してください。

## パラメータ説明
### X
描画画像サイズのXが最大画像サイズのX以上だった場合の、X軸方向のオフセット値です。
### Y
描画画像サイズのYが最大画像サイズのY以上だった場合の、Y軸方向のオフセット値です。
### Scale
拡大率です。
### Method
拡大・縮小する際の補間形式です。

|数字| アルゴリズム名 |
|----|----------------|
| 0  |Nearest neighbor|
| 1  |Bilinear        |

### DelCache (r15~)
チェックされている場合、指定ファイルのキャッシュを削除します。  
(オブジェクトの描画を試みない限りキャッシュ削除が行われないため、ご注意ください。)
