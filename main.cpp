#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

class $modify(BufferLayer, GJBaseGameLayer) {
	struct Fields {
		int buf[2] = {0, 0};        // frames left to retry the click (p1, p2)
		bool held[2] = {false, false}; // is the real button still down
		bool pendingRelease[2] = {false, false};
		bool replaying = false;
	};

	static bool on() {
		return Mod::get()->getSettingValue<bool>("enabled");
	}

	static int frames() {
		return (int)Mod::get()->getSettingValue<int64_t>("buffer-frames");
	}

	void handleButton(bool down, int button, bool isPlayer1) {
		auto f = m_fields.self();
		if (f->replaying || !on() || button != 1) {
			GJBaseGameLayer::handleButton(down, button, isPlayer1);
			return;
		}
		int i = isPlayer1 ? 0 : 1;
		if (down) {
			f->held[i] = true;
			f->buf[i] = frames();
			f->pendingRelease[i] = false;
		} else {
			f->held[i] = false;
			// released before the buffered click was used: release after replaying it
			if (f->buf[i] > 0) f->pendingRelease[i] = true;
		}
		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}

	static bool canAct(PlayerObject* p) {
		if (!p) return false;
		if (p->m_isOnGround) return true;
		return p->m_touchingRings && p->m_touchingRings->count() > 0;
	}

	void processCommands(float dt) {
		if (on()) {
			auto f = m_fields.self();
			PlayerObject* players[2] = {m_player1, m_player2};
			for (int i = 0; i < 2; i++) {
				if (f->pendingRelease[i] && f->buf[i] <= 0) {
					// replayed click done, now let go
					f->replaying = true;
					GJBaseGameLayer::handleButton(false, 1, i == 0);
					f->replaying = false;
					f->pendingRelease[i] = false;
				}
				if (f->buf[i] > 0) {
					f->buf[i]--;
					if (canAct(players[i])) {
						f->replaying = true;
						GJBaseGameLayer::handleButton(true, 1, i == 0);
						f->replaying = false;
						f->buf[i] = 0;
					}
				}
			}
		}
		GJBaseGameLayer::processCommands(dt);
	}
};
