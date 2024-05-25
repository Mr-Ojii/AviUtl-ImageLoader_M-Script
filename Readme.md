# ImageLoader_M
AviUtlで最大画像サイズを超える画像のロードを行うスクリプト

## 概要
抹茶鯖のAodaruma氏のプロトタイプをもとに作ってみたものを、ePi氏に添削していただいたものです。  
内部で4GB制限にかからないような画像のキャッシュを独自に行っています。

## 読み込み可能な画像形式
+ GDI+で読み込み可能な画像形式
+ exedit.aufと同一ディレクトリ内のSusie-Pluginにより、非圧縮RGBAで展開される画像

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
