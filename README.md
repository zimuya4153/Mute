# Mute - 禁言插件

## 简介
Mute 是一个用于 Minecraft 服务器的禁言插件，它可以帮助你管理服务器的禁言功能。你可以使用这个插件来禁言玩家、解除禁言操作。
> 温馨提示：兼容LSE的所有聊天插件

## 命令
- `/mute all <时间>` - 全体禁言/解禁
- `/mute player <玩家> <时间>` - 禁言玩家/解禁
> 时间填0为解除禁言 填负数则永久禁言 禁言时间单位是秒   

`别问我为什么不弄 类似xp命令那样的L后缀 和 把禁用命令加上指令，我懒(小声逼逼)`

## 配置文件(config.json)
```json
{
    "version": 1, // 配置文件版本(勿动)
    "all": 0, // 全体禁言时间(请用指令操作)
    "players": {}, // 玩家禁言时间(请用指令操作)
    "disabledCmd": [ // 禁言时禁止使用的指令
        "me",
        "msg",
        "tell",
        "w"
    ]
}
```

## 安装方法

- 手动安装
  - 前往[Releases](https://github.com/zimuya4153/Mute/releases)下载最新版本的`Mute-windows-x64.zip`
  - 解压`压缩包内的`文件夹到`./plugins/`目录
- Lip 安装
  - 输入命令`lip install -y github.com/zimuya4153/Mute`
- ~~一条龙安装~~
  - ~~去 Q 群，喊人，帮你安装~~