--[[
    original version license
    ----------------------------------------------------------
    Copyright (c) 2023 黒猫大福@roku10shi
    Released under the MIT license

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
    ----------------------------------------------------------
]]

local P = {}

P.name = "ImageLoader_M Dropper"

P.priority = 2000

local extensions = { "bmp", "jpg", "jpeg", "png", "tif", "tiff", "exif" }

function P.contains(tbl, value)
  for _, v in ipairs(tbl) do
      if v == value then
          return true
      end
  end
  return false
end

function P.ondragenter(files, state)
  -- GCMZDrops は exedit.aufと同一ディレクトリに置く必要があるため、これでよい
  local inipath = GCMZDrops.scriptdir().."..\\exedit.ini"
  local f = io.open(inipath,"r")
  if f ~= nil then
    io.close(f)
    -- iniがあるなら、iniを読んで拡張子を取得する
    local ini = GCMZDrops.inifile(inipath)
    for i, v in pairs(ini:keys("extension")) do
      if ini:get("extension", v, "") == "画像ファイル" then
        local ext = string.sub(v, 2)
        if not P.contains(extensions, ext) then
          table.insert(extensions, ext)
        end
      end
    end
  end

  for i, v in ipairs(files) do
    local ext = v.filepath:match("[^.]+$"):lower()
    for i, ext_ in pairs(extensions) do
      if ext_ == ext then
        print("拡張子:" .. ext)
        -- ファイルの拡張子が json のファイルがあったら処理できそうなので true
        return true
      end
    end
  end
  return false
end

function P.ondragover(files, state)
  -- ondragenterで処理できる、もしくは処理できそうな場合は true を返してください。
  return true
end

function P.ondragleave()
end

function P.encodelua(s)
  s = GCMZDrops.convertencoding(s, "sjis", "utf8")
  s = GCMZDrops.encodeluastring(s)
  s = GCMZDrops.convertencoding(s, "utf8", "sjis")
  return s
end

-- ondrop はマウスボタンが離され、ファイルがドロップされた時に呼ばれます。
-- ただし ondragenter や ondragover で false を返していた場合は呼ばれません。
function P.ondrop(files, state)
  if state["shift"] then
    local dropflag = false
    local proj = GCMZDrops.getexeditfileinfo()
    local jp = not GCMZDrops.englishpatched()
    local exo = [[
[exedit]
width=]] .. proj.width .. "\r\n" .. [[
height=]] .. proj.height .. "\r\n" .. [[
rate=]] .. proj.rate .. "\r\n" .. [[
scale=]] .. proj.scale .. "\r\n" .. [[
length=64
audio_rate=]] .. proj.audio_rate .. "\r\n" .. [[
audio_ch=]] .. proj.audio_ch .. "\r\n"
    for i, v in ipairs(files) do
      local ext = v.filepath:match("[^.]+$"):lower()
      for i_, ext_ in pairs(extensions) do
        if ext_ == ext then
          dropflag = true
          -- ファイルの拡張子が json のファイルがあったら処理できそうなので
          filepath = v.filepath
          -- ファイルを直接読み込む代わりに exo ファイルを組み立てる
          math.randomseed(os.time())
          exo = exo .. [[
[]] .. i - 1 .. [[]
start=1
end=128
layer=]] .. i .. "\r\n" .. [[
overlay=1
camera=0
[]] .. i - 1 .. [[.0]
_name=]] .. (jp and [[カスタムオブジェクト]] or [[Custom object]]) .. "\r\n" .. [[
track0=0.00
track1=0.00
track2=100.00
track3=0.00
check0=0
type=0
filter=2
name=ImageLoader_M
param=file=]] .. P.encodelua(filepath) .. "\r\n" .. [[
[]] .. i - 1 .. [[.1]
_name=]] .. (jp and [[標準描画]] or [[Standard drawing]]) .. "\r\n" .. [[
X=0.0
Y=0.0
Z=0.0
]] .. (jp and [[拡大率]] or [[Zoom%]]) .. [[=100.00
]] .. (jp and [[透明度]] or [[Clearness]]) .. [[=0.0
]] .. (jp and [[回転\]] or [[Rotation]]) .. [[=0.00
blend=0]] .. "\r\n"
        end
      end
    end
    print(exo)
    if dropflag then
      local filepath = GCMZDrops.createtempfile("ILM", ".exo")

      f, err = io.open(filepath, "wb")
      if f == nil then
        error(err)
      end
      f:write(exo)
      f:close()
      print("[" .. P.name .. "] が  を exo ファイルに差し替えました。元のファイルは orgfilepath で取得できます。")

      return { { filepath = filepath } }, state
    end
    -- 他のイベントハンドラーにも処理をさせたいのでここは常に false
  end
  return false
end

return P
