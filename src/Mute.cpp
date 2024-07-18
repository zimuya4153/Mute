#include "Mute.h"
#include "Config.h"
#include "magic_enum.hpp"

#include <chrono>
#include <ll/api/Config.h>
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/event/command/ExecuteCommandEvent.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/plugin/NativePlugin.h>
#include <ll/api/plugin/RegisterHelper.h>
#include <mc/network/NetworkIdentifier.h>
#include <mc/network/ServerNetworkHandler.h>
#include <mc/network/packet/TextPacket.h>
#include <mc/server/commands/CommandOriginType.h>
#include <mc/server/commands/CommandOutput.h>
#include <memory>


ll::Logger   logger("Mute");
Mute::Config config;

std::string timestampToString(int timestamp) {
    if (timestamp < 0) {
        return "永久";
    }
    time_t     time_t_timestamp = static_cast<time_t>(timestamp);
    struct tm* local_time       = std::localtime(&time_t_timestamp);
    char       time_string[26];
    std::strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", local_time);
    return std::string(time_string);
}

LL_TYPE_INSTANCE_HOOK(
    PlayerSendMessageHook,
    HookPriority::High,
    ServerNetworkHandler,
    &ServerNetworkHandler::handle,
    void,
    NetworkIdentifier const& identifier,
    TextPacket const&        packet
) {
    if (auto player = getServerPlayer(identifier, packet.mClientSubId); player && !player->isOperator()) {
        auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (config.all != 0) {
            if (config.all == -1 || config.all > time) {
                player->sendMessage(fmt::format("§e当前是全员禁言状态。解禁时间:{}", timestampToString(config.all)));
                return;
            }
            config.all = 0;
        }
        if (auto playerMuteTime = config.players[player->getUuid().asString()]; playerMuteTime) {
            if (playerMuteTime == -1 || playerMuteTime > time) {
                player->sendMessage(fmt::format("§e你已被禁言。解禁时间:{}", timestampToString(playerMuteTime)));
                return;
            }
            config.players.erase(player->getUuid().asString());
        }
        ll::config::saveConfig(config, Mute::mute::getInstance().getSelf().getConfigDir() / "config.json");
    }
    origin(identifier, packet);
}

bool startsWithAnyOf(std::string& str, const std::vector<std::string>& prefixes) {
    if (str.starts_with("/")) {
        str.erase(0, 1);
    }
    for (auto prefix : prefixes) {
        if (str.starts_with(prefix)) {
            return true;
        }
    }
    return false;
}
LL_TYPE_INSTANCE_HOOK(
    PlayerRunCmdHook,
    HookPriority::High,
    MinecraftCommands,
    &MinecraftCommands::executeCommand,
    MCRESULT,
    CommandContext& context,
    bool            suppressOutput
) {
    if (context.mOrigin->getOriginType() == CommandOriginType::Player
        && startsWithAnyOf(context.mCommand, config.disabledCmd)) {
        auto player = static_cast<Player*>(context.mOrigin->getEntity());
        if (player && !player->isOperator()) {
            auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            if (config.all != 0) {
                if (config.all == -1 || config.all > time) {
                    player->sendMessage(fmt::format("§e当前是全员禁言状态。解禁时间:{}", timestampToString(config.all))
                    );
                    return MCRESULT_CommandsDisabled;
                }
                config.all = 0;
            }
            if (auto playerMuteTime = config.players[player->getUuid().asString()]; playerMuteTime) {
                if (playerMuteTime == -1 || playerMuteTime > time) {
                    player->sendMessage(fmt::format("§e你已被禁言。解禁时间:{}", timestampToString(playerMuteTime)));
                    return MCRESULT_CommandsDisabled;
                }
                config.players.erase(player->getUuid().asString());
            }
        }
    }
    return origin(context, suppressOutput);
}

namespace Mute {

static std::unique_ptr<mute> instance;

mute& mute::getInstance() { return *instance; }

bool mute::load() {
    const auto& configFilePath = getSelf().getConfigDir() / "config.json";
    try {
        if (!ll::config::loadConfig(config, configFilePath)) {
            logger.warn("无法加载配置文件 {}", configFilePath);
            logger.info("尝试创建默认配置文件");
            if (!ll::config::saveConfig(config, configFilePath)) {
                logger.error("无法写入默认配置 {}", configFilePath);
            }
        }
    } catch (...) {
        logger.warn("无法加载配置文件 {}", configFilePath);
        logger.info("尝试创建默认配置文件");
        if (!ll::config::saveConfig(config, configFilePath)) {
            logger.error("无法写入默认配置 {}", configFilePath);
        }
        return false;
    }
    return true;
}

bool mute::enable() {
    ll::memory::HookRegistrar<PlayerSendMessageHook>().hook();
    ll::memory::HookRegistrar<PlayerRunCmdHook>().hook();
    auto& cmd = ll::command::CommandRegistrar::getInstance()
                    .getOrCreateCommand("mute", "禁言", CommandPermissionLevel::GameDirectors);
    struct cmdParam {
        CommandSelector<Player> player;
        int                     time;
    };
    cmd.overload<cmdParam>().text("all").required("time").execute(
        [&](CommandOrigin const& origin, CommandOutput& output, cmdParam const& results) {
            if (results.time == 0) {
                config.all = 0;
                output.success("§a已解除全员禁言");
                return;
            }
            config.all = results.time < 0 ? -1
                                          : std::chrono::system_clock::to_time_t(
                                                std::chrono::seconds(results.time) + std::chrono::system_clock::now()
                                            );
            output.success(fmt::format("§e全员禁言已开启。解禁时间:{}", timestampToString(config.all)));
            ll::config::saveConfig(config, getSelf().getConfigDir() / "config.json");
        }
    );
    cmd.overload<cmdParam>().text("player").required("player").required("time").execute(
        [&](CommandOrigin const& origin, CommandOutput& output, cmdParam const& results) {
            if (results.player.results(origin).empty()) {
                output.error("未选择玩家。");
                return;
            }
            for (auto* player : results.player.results(origin)) {
                auto uid = player->getUuid().asString();
                if (results.time == 0) {
                    config.players.erase(uid);
                    output.success(fmt::format("§e已解除玩家{}的禁言", player->getRealName()));
                    continue;
                }
                if (player->isOperator()) {
                    output.error(fmt::format("§c玩家{}是管理员，无法禁言", player->getRealName()));
                    continue;
                }
                config.players[uid] = results.time < 0
                                        ? -1
                                        : std::chrono::system_clock::to_time_t(
                                              std::chrono::seconds(results.time) + std::chrono::system_clock::now()
                                          );
                output.success(fmt::format(
                    "§e玩家{}的已被禁言。解禁时间:{}",
                    player->getRealName(),
                    timestampToString(config.players[uid])
                ));
            }
            ll::config::saveConfig(config, Mute::mute::getInstance().getSelf().getConfigDir() / "config.json");
        }
    );
    return true;
}

bool mute::disable() {
    ll::memory::HookRegistrar<PlayerSendMessageHook>().unhook();
    ll::memory::HookRegistrar<PlayerRunCmdHook>().unhook();
    ll::config::saveConfig(config, getSelf().getConfigDir() / "config.json");
    return true;
}

bool mute::unload() {
    ll::memory::HookRegistrar<PlayerSendMessageHook>().unhook();
    ll::memory::HookRegistrar<PlayerRunCmdHook>().unhook();
    ll::config::saveConfig(config, getSelf().getConfigDir() / "config.json");
    return true;
}

} // namespace Mute

LL_REGISTER_PLUGIN(Mute::mute, Mute::instance);