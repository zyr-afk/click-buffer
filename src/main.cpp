#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

static int g_step = 0;

static bool canJumpNow(PlayerObject* p) {
    auto mod = Mod::get();
    if (mod->getSettingValue<bool>("buffer-ground") && p->m_isOnGround) return true;
    if (mod->getSettingValue<bool>("buffer-orbs") && p->m_touchingRings->count() > 0) return true;
    return false;
}

class $modify(BufferPlayer, PlayerObject) {
    struct Fields {
        bool pending = false;
        int pressStep = 0;
        bool realHeld = false;
        bool firedByMod = false;
        bool injecting = false;
    };

    bool pushButton(PlayerButton btn) {
        if (m_fields->injecting || btn != PlayerButton::Jump ||
            !Mod::get()->getSettingValue<bool>("enabled")) {
            return PlayerObject::pushButton(btn);
        }
        m_fields->realHeld = true;
        m_fields->firedByMod = false;
        bool ready = canJumpNow(this);
        if (!ready) {
            m_fields->pending = true;
            m_fields->pressStep = g_step;
        }
        return PlayerObject::pushButton(btn);
    }

    bool releaseButton(PlayerButton btn) {
        if (!m_fields->injecting && btn == PlayerButton::Jump) {
            m_fields->realHeld = false;
            m_fields->pending = false;
        }
        return PlayerObject::releaseButton(btn);
    }

    void tick() {
        auto f = m_fields.self();
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;

        if (f->firedByMod && !f->realHeld) {
            f->firedByMod = false;
            f->injecting = true;
            PlayerObject::releaseButton(PlayerButton::Jump);
            f->injecting = false;
        }
        if (!f->pending) return;

        int window = (int)Mod::get()->getSettingValue<int64_t>("buffer-frames");
        if (g_step - f->pressStep > window) {
            f->pending = false;
            return;
        }
        if (canJumpNow(this)) {
            f->pending = false;
            f->firedByMod = true;
            f->injecting = true;
            PlayerObject::pushButton(PlayerButton::Jump);
            f->injecting = false;
        }
    }
};

class $modify(BufferLayer, GJBaseGameLayer) {
    void processCommands(float dt) {
        g_step++;
        GJBaseGameLayer::processCommands(dt);
        if (m_player1) static_cast<BufferPlayer*>(m_player1)->tick();
        if (m_player2) static_cast<BufferPlayer*>(m_player2)->tick();
    }
};
