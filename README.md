# RecottePlugin

RecotteStudioのプラグインです。

![](img/ss1.gif)

## 対応バージョン

- =v1.3.7.1
- =v1.3.7.2

## 使い方

1. RecotteStudioをインストール

1. [こちら](https://github.com/wallstudio/RecottePlugin/releases/)から最新版をダウンロードして適当なところに展開

1. RecotteStudioのあるフォルダに中身をコピー（`RecotteStudio.exe`と`Plugins`フォルダが同じ階層になるように）

1. レコスタを起動すると、Pluginsの中のプラグインが読み込まれます。

#### LayerFolding

レイヤーのラベル（左側のペイン）の左上にある■をクリックするとそのレイヤーの折り畳み状態をON/OFFできます。反映は、次の再描画のタイミング（タイムラインをクリックしたときなど）からとなっています。

話者レイヤーと注釈レイヤーが対応しています。注釈レイヤーは画像を重ねがちなので活用しやすいかも。

レイヤー内のオブジェクトのクリッピングはされていないので、下のレイヤーの背面に透けて見えていますがこれは仕様です。

#### CustomSkin

Pluginフォルダ内に`skin.png`の名前で画像を入れておくことでタイムラインに召喚することができます。  

デフォルトで私服のマキさんが描かれるようになっています。かわいいね！

## 機能

* Pluginのロード（`d3d11.dll`）

* レイヤーの折り畳み機能（`LayerFolding.dll`）

* タイムラインにマキマキを描画（`CustomSkin.dll`）

## ビルド環境

- VisualStudio2019 16.10.2

`std::format`とか使ってるので少し前のバージョンでも動かないと思います。

`link.bat`を実行して開発用ディレクトリとRecotteStudioのインストールディレクトリにリンクを貼ってください。VisualStudioでのビルドの際には開発用ディレクトリにのみコピーされるようになっています。（UACがうざいので）

## 開発方法

`LayerFolding.cpp`あたりを参考にして、`OnPluginStart`と`OnPluginFinish`をエクスポートしたdllを作ってください。

結局のところRecotteStudio内部の処理をもりもり書き換えないことには何もできないので覚悟が必要です。一応動的に書き換えるためのUtilityが`HookHelper`で実装されているので必要に応じて使ってください。

## メモ

作りとしてはWin32のWindowシステムベースだけど、GDI+で独自の描画をしている個所が多いのでUIいじる系は結構大変？

- https://qiita.com/up-hash/items/28375739208402721323
- https://qiita.com/up-hash/items/8ca41c4038c26a96674a
- https://gist.github.com/wallstudio/b78ce70e015058f7c33e391b0cfd7815
- https://silight.hatenablog.jp/entry/2016/08/23/212820
