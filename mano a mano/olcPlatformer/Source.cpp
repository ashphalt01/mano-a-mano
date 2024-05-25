#define OLC_PGE_APPLICATION
#define OLC_PGEX_ANIMSPR
#include "olcPixelGameEngine.h"
#include "olcPGEX_AnimatedSprite.h"
#include <iostream>
#include <string>
#include <math.h>
using namespace std;


// the size of a virtual pixel
int olcPixelDimen = 4;

class Engine : public olc::PixelGameEngine
{
public:
	Engine()
	{
		sAppName = "Mano a Mano";
	}
	olc::Sprite *levelSurface = nullptr, *levelBottom = nullptr, *levelBushA = nullptr, *playerA = nullptr, *playerB = nullptr, *aReadySword = nullptr, *angledSword = nullptr;
	olc::Sprite *volcanoBackground = nullptr, *playerADead = nullptr, *playerBDead = nullptr;

private:
	wstring middleSceneVolcanoLevel;
	// vector stores the properties of dead players on the level, where [0] is x coord, [1] y coord, [2] player a or b corpse, [3] x velocity, [4] y velocity 
	vector<vector<float>> vMiddleSceneVolcanoLevelDeadPlayers = {};
	//olc::AnimatedSprite volcanoBackground;
	bool theGameBegins = false;

	float fPlayerAPosX = 19.0f, fPlayerAPosY = 8.0f;
	float fPlayerAVelX = 0.0f, fPlayerAVelY = 0.0f;
	float fPlayerBPosX = 45.0f, fPlayerBPosY = 8.0f;
	float fPlayerBVelX = 0.0f, fPlayerBVelY = 0.0f;
	bool bPlayerAGrounded = false, bPlayerBGrounded = false;
	bool bPlayerAAlive = false, bPlayerBAlive = false;
	bool bFacingEachOther;
	float fPlayerARespawn = 3.0f, fPlayerBRespawn = 3.0f;
	int iPlayerADirection, iPlayerBDirection;

	float fCameraPosX = 0.0f, fCameraPosY = 0.0f;
	int iLevelWidth, iLevelHeight;

	float fSwordPointAX = 1.0f, fSwordPointAY = 1.0f;
	float fSwordPointBX = 1.0f, fSwordPointBY = 1.0f;
	int iSwordStatePlayerA, iSwordStatePlayerB;
	float fLungeCooldownPlayerA = 1.0f, fLungeCooldownPlayerB = 1.0f;
	float bLungingA = 0.0f, bLungingB = 0.0f;

	bool wireframeMode;
	bool debugMode = false;

private:
	virtual bool OnUserCreate()
	{
		iLevelWidth = 64;
		iLevelHeight = 16;
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L"..S.......................###########...........................";
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L".................S.....................S............S...........";
		middleSceneVolcanoLevel += L"................................................................";
		middleSceneVolcanoLevel += L"########................................................########";
		middleSceneVolcanoLevel += L"BBBBBBBB########................................########BBBBBBBB";
		middleSceneVolcanoLevel += L"BBBBBBBBBBBBBBBB########................########BBBBBBBBBBBBBBBB";
		middleSceneVolcanoLevel += L"BBBBBBBBBBBBBBBBBBBBBBBB################BBBBBBBBBBBBBBBBBBBBBBBB";
		middleSceneVolcanoLevel += L"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
		middleSceneVolcanoLevel += L"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
		
		//volcanoBackground.mode = olc::AnimatedSprite::SPRITE_MODE::MULTI;
		////volcanoBackground.SetSpriteSize( 480.0f, 270.0f);
		//volcanoBackground.SetSpriteScale(2.0f);
		//volcanoBackground.AddState("default", 0.1f, olc::AnimatedSprite::PLAY_MODE::PING_PONG, {
		//	"Volcano anim. 01.png"
		//	"Volcano anim. 02.png"
		//	"Volcano anim. 03.png"
		//	});
		//volcanoBackground.SetState("default");
		volcanoBackground = new olc::Sprite("./sprites/Volcano/Volcano anim. 01.png");
		levelSurface = new olc::Sprite("./sprites/levelSurface/grassTop.png");
		levelBottom = new olc::Sprite("./sprites/levelSurface/dirtBottom.png");
		levelBushA = new olc::Sprite("./sprites/levelSurface/bushA.png");
		playerA = new olc::Sprite("./sprites/playerA.png");
		playerB = new olc::Sprite("./sprites/playerB.png");
		aReadySword = new olc::Sprite("./sprites/swordPresent.png");
		angledSword = new olc::Sprite("./sprites/swordAngled.png");
		playerADead = new olc::Sprite("./sprites/playerADed.png");
		playerBDead = new olc::Sprite("./sprites/playerBDed.png");

		return true;
	}

	virtual bool OnUserUpdate(float fElapsedTime)
	{
		// lambda function for getting and setting tiles
		auto getTile = [&](int x, int y) {
			if (x >= 0 && x < iLevelWidth && y >= 0 && y < iLevelHeight) {
				return middleSceneVolcanoLevel[y * iLevelWidth + x];
			}
			else {
				return L' ';
			}
		};
		// limiting velocity
		auto limitVelocity = [&](float &fPlayerVelX, float &playerVelY) {
			if (fPlayerVelX > 15.0f) fPlayerVelX -= 2.0f;
			if (fPlayerVelX < -15.0f) fPlayerVelX += 2.0f;
			if (playerVelY > 30.0f) playerVelY = 30.0f;
			if (playerVelY < -20.0f) playerVelY = -20.0f;
			return;
		};
		auto lungeDisplayCooldown = [&](float &fLungeCooldown, float fElapsedTime) {
			fLungeCooldown -= fElapsedTime;
			if (fLungeCooldown < 0.0f) fLungeCooldown = 0.0f;
		};
		// drawing players and sword in player hands
		auto drawAlivePlayers = [&](bool &bPlayerAlive, float &fRespawnTime, float fElapsedTime, float &vSwordPointX, float &vSwordPointY, float &fPlayerPosX, float &fPlayerPosY, float fOffsetX, float fOffsetY, int nTileWidth, olc::Sprite* playerAliveSprite,
			olc::Sprite* readySword, olc::Sprite* angledSword, int iDirMod, int iSwordState, float &lungeDisplay, bool debugMode, bool AorB) {
				if (bPlayerAlive) {
					DrawSprite((fPlayerPosX - fOffsetX) * nTileWidth, (fPlayerPosY - fOffsetY) * nTileWidth, playerAliveSprite, 1, (AorB ? ((iDirMod == 1 ? 1 : 0)) : ((iDirMod == 1 ? 0 : 1))));
					SetPixelMode(olc::Pixel::MASK);
					// drawing swords, where a sword state of 1 is downward, 0 is upward and 2 is straight
					switch (iSwordState) {
					case 2:
						if (lungeDisplay > 0.0f) {
							DrawSprite((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -2.0f : 1.55f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.1f) * nTileWidth, readySword, 1, (iDirMod == 1 ? 3 : 2));
							lungeDisplayCooldown(lungeDisplay, fElapsedTime);
							vSwordPointX = (fPlayerPosX + (iDirMod == 1 ? -2.0f : 3.0f)) * nTileWidth;
							vSwordPointY = (fPlayerPosY + 1.3f) * nTileWidth;
							if (debugMode) Draw((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -2.0f : 3.0f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.3f) * nTileWidth, olc::RED);

						}
						else {
							DrawSprite((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -1.0f : 0.55f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.1f) * nTileWidth, readySword, 1, (iDirMod == 1 ? 3 : 2));
							vSwordPointX = (fPlayerPosX + (iDirMod == 1 ? -1.0f : 2.0f)) * nTileWidth;
							vSwordPointY = (fPlayerPosY + 1.3f) * nTileWidth;
							if (debugMode) Draw((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -1.0f : 2.0f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.3f) * nTileWidth, olc::RED);
						}
						break;
					case 0:
						if (lungeDisplay > 0.0f) {
							DrawSprite((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -2.0f : 1.55f)) * nTileWidth, (fPlayerPosY - fOffsetY + 0.1f) * nTileWidth, readySword, 1, (iDirMod == 1 ? 3 : 2));
							lungeDisplayCooldown(lungeDisplay, fElapsedTime);
							if (debugMode) Draw((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -2.0f : 3.0f)) * nTileWidth, (fPlayerPosY - fOffsetY + 0.3f) * nTileWidth, olc::RED);
							vSwordPointX = (fPlayerPosX + (iDirMod == 1 ? -2.0f : 3.0f)) * nTileWidth;
							vSwordPointY = (fPlayerPosY + 0.3f) * nTileWidth;
						}
						else {
							DrawSprite((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -0.5f : 0.5f)) * nTileWidth, (fPlayerPosY - fOffsetY + 0.5f) * nTileWidth, angledSword, 1, (iDirMod == 1 ? 1 : 0));
							if (debugMode) Draw((fPlayerPosX -fOffsetX + (iDirMod == 1 ? -0.5f : 1.5f)) * nTileWidth, (fPlayerPosY - fOffsetY + 0.3f) * nTileWidth, olc::RED);
							vSwordPointX = (fPlayerPosX + (iDirMod == 1 ? -0.5f : 1.5f)) * nTileWidth;
							vSwordPointY = (fPlayerPosY + 0.3f) * nTileWidth;
						}
						break;
					case 1:
						if (lungeDisplay > 0.0f) {
							DrawSprite((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -2.0f : 1.55f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.4f) * nTileWidth, readySword, 1, (iDirMod == 1 ? 3 : 2));
							lungeDisplayCooldown(lungeDisplay, fElapsedTime);
							if (debugMode) Draw((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -2.0f : 3.0f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.8f) * nTileWidth, olc::RED);
							vSwordPointX = (fPlayerPosX + (iDirMod == 1 ? -2.0f : 3.0f)) * nTileWidth;
							vSwordPointY = (fPlayerPosY + 1.8f) * nTileWidth;
						}
						else {
							DrawSprite((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -0.5f : 0.5f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.3f) * nTileWidth, angledSword, 1, (iDirMod == 1 ? 3 : 2));
							if (debugMode) Draw((fPlayerPosX - fOffsetX + (iDirMod == 1 ? -0.5f : 1.5f)) * nTileWidth, (fPlayerPosY - fOffsetY + 1.8f) * nTileWidth, olc::RED);
							vSwordPointX = (fPlayerPosX + (iDirMod == 1 ? -0.5f : 1.5f)) * nTileWidth;
							vSwordPointY = (fPlayerPosY + 1.8f) * nTileWidth;
						}
						break;
					}
					SetPixelMode(olc::Pixel::NORMAL);
				}
				else {
					fRespawnTime -= fElapsedTime;
					if (fRespawnTime <= 0.0f) {
						bPlayerAlive = true;
						fPlayerPosX = 32.0f;
						fPlayerPosY = 2.0f;
						fRespawnTime = 3.0f;
					}
				}
		};
		// collision detection of players with the level, using truncation to test corners of the character hitboxes
		auto detectPlayerCollisionWithLevel = [&](float& fNewPlayerPosX, float& fPlayerPosY, float& fPlayerVelX, float& fNewPlayerPosY, float& fPlayerVelY, bool& playerGrounded) {
			if (fPlayerVelX <= 0) { // moving left
				if (getTile(fNewPlayerPosX + 0.0f, fPlayerPosY + 0.0f) == L'#' || getTile(fNewPlayerPosX + 0.0f, fPlayerPosY + 1.9f) == L'#' || getTile(fNewPlayerPosX + 0.0f, fPlayerPosY + 0.9f) == L'#') {
					fNewPlayerPosX = (int)fNewPlayerPosX + 1;
					fPlayerVelX = 0;
				}
			}
			else { // moving right
				if (getTile(fNewPlayerPosX + 0.9f, fPlayerPosY + 0.0f) == L'#' || getTile(fNewPlayerPosX + 0.9f, fPlayerPosY + 1.9f) == L'#' || getTile(fNewPlayerPosX + 0.9f, fPlayerPosY + 0.9f) == L'#') {
					fNewPlayerPosX = (int)fNewPlayerPosX;
					fPlayerVelX = 0;
				}
			}
			playerGrounded = false;
			if (fPlayerVelY <= 0) { // moving up
				if (getTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY) == L'#' || getTile(fNewPlayerPosX + 0.9f, fNewPlayerPosY) == L'#') {
					fNewPlayerPosY = (int)fNewPlayerPosY + 1;
					fPlayerVelY = 0;
				}
			}
			else { // moving down
				if (getTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY + 2.0f) == L'#' || getTile(fNewPlayerPosX + 0.9f, fNewPlayerPosY + 2.0f) == L'#') {
					fNewPlayerPosY = (int)fNewPlayerPosY;
					fPlayerVelY = 0;
					playerGrounded = true;
				}
			}
		};
		auto detectCorpseCollisionWithLevel = [&](float& fNewPlayerPosX, float& fPlayerPosY, float& fPlayerVelX, float& fNewPlayerPosY, float& fPlayerVelY) {
			if (fPlayerVelX <= 0) { // moving left
				if (getTile(fNewPlayerPosX + 0.0f, fPlayerPosY + 0.0f) == L'#' || getTile(fNewPlayerPosX + 0.0f, fPlayerPosY + 0.9f) == L'#') {
					fNewPlayerPosX = (int)fNewPlayerPosX + 1;
					fPlayerVelX = 0;
				}
			}
			else { // moving right
				if (getTile(fNewPlayerPosX + 0.9f, fPlayerPosY + 0.0f) == L'#' || getTile(fNewPlayerPosX + 0.9f, fPlayerPosY + 0.9f) == L'#') {
					fNewPlayerPosX = (int)fNewPlayerPosX;
					fPlayerVelX = 0;
				}
			}
			if (fPlayerVelY <= 0) { // moving up
				if (getTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY) == L'#' || getTile(fNewPlayerPosX + 1.9f, fNewPlayerPosY) == L'#') {
					fNewPlayerPosY = (int)fNewPlayerPosY + 1;
					fPlayerVelY = 0;
				}
			}
			else { // moving down
				if (getTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY + 1.0f) == L'#' || getTile(fNewPlayerPosX + 1.9f, fNewPlayerPosY + 1.0f) == L'#') {
					fNewPlayerPosY = (int)fNewPlayerPosY;
					fPlayerVelY = 0;
				}
			}
		};
		// collision detection of players with opposite player sword. sword point is reset when a player dies, to eliminate the killed player killing the killing player after death
		auto detectCollisionWithSwords = [&](vector<vector<float>> &vDeadPlayers, int iPlayerBDirection, int iPlayerADirection, float fSwordPointAX, float fSwordPointAY, float &fSwordPointBX, float &fSwordPointBY, bool &bPlayerAlive, float &fPlayerPosX, 
			float fPlayerPosY, float &fVelAX, float &fVelAY, float &fVelBX, float &fVelBY, int nTileWidth, bool debugMode, bool AorB) {
			if ((fSwordPointAX > fPlayerPosX * nTileWidth) && (fSwordPointAX < (fPlayerPosX + 0.9f) * nTileWidth) && (fSwordPointAY > fPlayerPosY * nTileWidth) && (fSwordPointAY < (fPlayerPosY + 1.9f) * nTileWidth) && bPlayerAlive) {
				if (fSwordPointAY == fSwordPointBY && iPlayerADirection != iPlayerBDirection) {
					fVelAX *= -1.0f;
					fVelAY -= 7.0f;
					fVelBX *= -1.0f;
					fVelBY -= 7.0f;
					iPlayerBDirection > 1 ? fPlayerPosX -= 0.3f : fPlayerPosX += 0.3f;
					return;
				}
				bPlayerAlive = false;
				fSwordPointBX = -1.0f;
				fSwordPointBY = -1.0f;
				vDeadPlayers.push_back({ fPlayerPosX, fPlayerPosY, (AorB ? 2.0f : 1.0f), (iPlayerBDirection == 2 ? 20.0f : -20.0f), -10.0f});
			}
		};
		// drawing player corpses
		auto drawDeadPlayers = [&](vector<vector<float>> &vDeadPlayers, olc::Sprite* playerADeadSprite, olc::Sprite* playerBDeadSprite, float fOffsetX, float fOffsetY, float fElapsedTime) {
			for (int i = 0; i < vDeadPlayers.size(); i++) {
				float player = vDeadPlayers[i].at(2);
				float fNewVelX = vDeadPlayers[i].at(3) + (-4.0f * (vDeadPlayers[i].at(3)) * fElapsedTime);
				if (fabs(fNewVelX) < 0.01f) fNewVelX = 0.0f;
				float fNewVelY = vDeadPlayers[i].at(4) + 40.0f * fElapsedTime;
				float fNewPosX = vDeadPlayers[i].at(0) + fNewVelX * fElapsedTime;
				float fNewPosY = vDeadPlayers[i].at(1) + fNewVelY * fElapsedTime;
				detectCorpseCollisionWithLevel(fNewPosX, vDeadPlayers[i].at(1), fNewVelX, fNewPosY, fNewVelY);
				vDeadPlayers[i].at(3) = fNewVelX;
				vDeadPlayers[i].at(4) = fNewVelY;
				vDeadPlayers[i].at(0) = fNewPosX;
				vDeadPlayers[i].at(1) = fNewPosY;
				DrawSprite((fNewPosX - fOffsetX) * 16, (fNewPosY - fOffsetY) * 16, (player == 1.0f ? playerADeadSprite : playerBDeadSprite), 1, 0);
			}
		};
		fLungeCooldownPlayerA > 0.0f ? fLungeCooldownPlayerA -= fElapsedTime : fLungeCooldownPlayerA = 0.0f;
		fLungeCooldownPlayerB > 0.0f ? fLungeCooldownPlayerB -= fElapsedTime : fLungeCooldownPlayerB = 0.0f;

		// input handling
		if (IsFocused()) {
			// player a input
			if (bPlayerAAlive == true) {
				if (GetKey(olc::Key::W).bHeld) {
					iSwordStatePlayerA = 0;
				}
				else if (GetKey(olc::Key::S).bHeld) {
					iSwordStatePlayerA = 1;
				}
				else iSwordStatePlayerA = 2;
				if (GetKey(olc::Key::A).bHeld) {
					fPlayerAVelX += (bPlayerAGrounded ? -60.0f : -2.0f) * fElapsedTime;
					iPlayerADirection = 1;
				}
				if (GetKey(olc::Key::D).bHeld) {
					fPlayerAVelX += (bPlayerAGrounded ? 60.0f : 2.0f) * fElapsedTime;
					iPlayerADirection = 2;
				}
				if (GetKey(olc::Key::V).bPressed) {
					if (fPlayerAVelY == 0) fPlayerAVelY = -15.0f;
				}
				if (GetKey(olc::Key::B).bPressed && fLungeCooldownPlayerA <= 0.0f && GetKey(olc::Key::D).bHeld) {
					fLungeCooldownPlayerA = 1.0f;
					fPlayerAVelX += 30.0f;
					bLungingA = 0.5f;
				}
				if (GetKey(olc::Key::B).bPressed && fLungeCooldownPlayerA <= 0.0f && GetKey(olc::Key::A).bHeld) {
					fLungeCooldownPlayerA = 1.0f;
					fPlayerAVelX -= 30.0f;
					bLungingA = 0.5f;
				}
			}
			// now player b input
			if (bPlayerBAlive == true) {
				if (GetKey(olc::Key::I).bHeld) {
					iSwordStatePlayerB = 0;
				}
				else if (GetKey(olc::Key::K).bHeld) {
					iSwordStatePlayerB = 1;
				}
				else iSwordStatePlayerB = 2;
				if (GetKey(olc::Key::J).bHeld) {
					fPlayerBVelX += (bPlayerBGrounded ? -60.0f : -2.0f) * fElapsedTime;
					iPlayerBDirection = 1;
				}
				if (GetKey(olc::Key::L).bHeld) {
					fPlayerBVelX += (bPlayerBGrounded ? 60.0f : 2.0f) * fElapsedTime;
					iPlayerBDirection = 2;
				}
				if (GetKey(olc::Key::ENTER).bPressed) {
					if (fPlayerBVelY == 0) fPlayerBVelY = -15.0f;
				}
				if (GetKey(olc::Key::SHIFT).bPressed && fLungeCooldownPlayerB <= 0.0f && GetKey(olc::Key::L).bHeld) {
					fLungeCooldownPlayerB = 1.0f;
					fPlayerBVelX += 30.0f;
					bLungingB = 0.5f;
				}
				if (GetKey(olc::Key::SHIFT).bPressed && fLungeCooldownPlayerB <= 0.0f && GetKey(olc::Key::J).bHeld) {
					fLungeCooldownPlayerB = 1.0f;
					fPlayerBVelX -= 30.0f;
					bLungingB = 0.5f;
				}
			}
			bFacingEachOther = (iPlayerADirection == iPlayerBDirection ? true : false);
			if (GetKey(olc::Key::F1).bPressed) debugMode = (debugMode ? false : true);
			if (debugMode && GetKey(olc::Key::Q).bPressed) wireframeMode = (wireframeMode ? false : true);
			if (debugMode && GetKey(olc::Key::R).bPressed) {
				for (std::vector<std::vector<float>>::size_type i = 0; i < vMiddleSceneVolcanoLevelDeadPlayers.size(); i++) {
					for (std::vector<int>::size_type j = 0; j < vMiddleSceneVolcanoLevelDeadPlayers[i].size(); j++)
					{
						std::cout << vMiddleSceneVolcanoLevelDeadPlayers[i][j] << endl;
					}
				}
			};
			if (debugMode && GetKey(olc::Key::T).bPressed) cout << fPlayerBRespawn << endl;
		}

		// gravity and drag calculations
		fPlayerAVelY += 40.0f * fElapsedTime;
		if (bPlayerAGrounded) {
			fPlayerAVelX += -8.0f * fPlayerAVelX * fElapsedTime;
			if (fabs(fPlayerAVelX) < 0.01f) fPlayerAVelX = 0.0f;
		}
		fPlayerBVelY += 40.0f * fElapsedTime;
		if (bPlayerBGrounded) {
			fPlayerBVelX += -8.0f * fPlayerBVelX * fElapsedTime;
			if (fabs(fPlayerBVelX) < 0.01f) fPlayerBVelX = 0.0f;
		}

		// call to limit velocity
		limitVelocity(fPlayerAVelX, fPlayerAVelY);
		limitVelocity(fPlayerBVelX, fPlayerBVelY);

		// new positions of the player
		float fNewPlayerAPosX = fPlayerAPosX + fPlayerAVelX * fElapsedTime;
		float fNewPlayerAPosY = fPlayerAPosY + fPlayerAVelY * fElapsedTime;
		float fNewPlayerBPosX = fPlayerBPosX + fPlayerBVelX * fElapsedTime;
		float fNewPlayerBPosY = fPlayerBPosY + fPlayerBVelY * fElapsedTime;

		// collision detection with the level, using truncated values to determine whether the tested player is on a solid block.
		// each corner is tested for whether it is intersecting a solid block
		detectPlayerCollisionWithLevel(fNewPlayerAPosX, fPlayerAPosY, fPlayerAVelX, fNewPlayerAPosY, fPlayerAVelY, bPlayerAGrounded);
		detectPlayerCollisionWithLevel(fNewPlayerBPosX, fPlayerBPosY, fPlayerBVelX, fNewPlayerBPosY, fPlayerBVelY, bPlayerBGrounded);

		fPlayerAPosX = fNewPlayerAPosX;
		fPlayerAPosY = fNewPlayerAPosY;
		fPlayerBPosX = fNewPlayerBPosX;
		fPlayerBPosY = fNewPlayerBPosY;

		// calculating the visible tilemap
		// NOTE : offsets are in virtual tiles
		fCameraPosX = (fPlayerAPosX + fPlayerBPosX) / 2;
		fCameraPosY = (fPlayerAPosY + fPlayerBPosY) / 2;
		int nTileWidth = 16;
		int nTileHeight = 16;
		int nVisibleTilesX = ScreenWidth() / nTileWidth;
		int nVisibleTilesY = ScreenHeight() / nTileHeight;

		float fOffsetX = fCameraPosX - (float)nVisibleTilesX / 2.0f;
		float fOffsetY = fCameraPosY - (float)nVisibleTilesY / 2.0f;

		// clamping camera to the game boundary
		if (fOffsetX < 0) fOffsetX = 0;
		if (fOffsetY < 0) fOffsetY = 0;
		if (fOffsetX > iLevelWidth - nVisibleTilesX) fOffsetX = iLevelWidth - nVisibleTilesX;
		if (fOffsetY > iLevelHeight - nVisibleTilesY) fOffsetY = iLevelHeight - nVisibleTilesY;

		// offsetting for smooth movement of camera
		// NOTE : offsets are in virtual tiles
		float fTileOffsetX = (fOffsetX - (int)fOffsetX) * nTileWidth;
		float fTileOffsetY = (fOffsetY - (int)fOffsetY) * nTileHeight;

		// collision detection of swords with the opposite player. the result is the living status of the tested players
		// swords are treated like an estoc ; ie, edgeless and damaging with the point
		detectCollisionWithSwords(vMiddleSceneVolcanoLevelDeadPlayers, iPlayerBDirection, iPlayerADirection, fSwordPointAX, fSwordPointAY, fSwordPointBX, fSwordPointBY, bPlayerBAlive, fPlayerBPosX, fPlayerBPosY, fPlayerAVelX, fPlayerAVelY, fPlayerBVelX, fPlayerBVelY, nTileWidth, debugMode, true);
		detectCollisionWithSwords(vMiddleSceneVolcanoLevelDeadPlayers, iPlayerADirection, iPlayerBDirection, fSwordPointBX, fSwordPointBY, fSwordPointAX, fSwordPointAY, bPlayerAAlive, fPlayerAPosX, fPlayerAPosY, fPlayerBVelX, fPlayerBVelY, fPlayerAVelX, fPlayerAVelY, nTileWidth, debugMode, false);
		// call to limit velocity
		limitVelocity(fPlayerAVelX, fPlayerAVelY);
		limitVelocity(fPlayerBVelX, fPlayerBVelY);

		// background image
		// volcanoBackground.Draw(fElapsedTime, { 0.0f, 0.0f }, olc::Sprite::Flip::NONE);
		DrawSprite(0, 0, volcanoBackground, roundf((float)(1920 / olcPixelDimen) / (float)(volcanoBackground->width)));

		// drawing the visible tiles
		for (int x = 0; x < nVisibleTilesX + 1; x++) {
			for (int y = 0; y < nVisibleTilesY + 1; y++) {
				wchar_t tile = getTile(x + fOffsetX, y + fOffsetY);
				switch (tile) {
				case L'#': // surface
					DrawPartialSprite(x * nTileWidth - fTileOffsetX, y * nTileHeight - fTileOffsetY, levelSurface, 0, 0, nTileWidth, nTileHeight);
					break;
				case L'B': // under surface
					DrawPartialSprite(x * nTileWidth - fTileOffsetX, y * nTileHeight - fTileOffsetY, levelBottom, 0, 0, nTileWidth, nTileHeight);
					break;
				case L'S': // grass bush
					SetPixelMode(olc::Pixel::MASK);
					DrawPartialSprite(x * nTileWidth - fTileOffsetX, y * nTileHeight - fTileOffsetY, levelBushA, 0, 0, nTileWidth * 8, nTileHeight * 4);
					SetPixelMode(olc::Pixel::NORMAL);
					break;
				}
			}
		}

		if (!theGameBegins) {
			DrawString(110, 100, "MANO A MANO", olc::RED, 3);
			DrawString(110, 200, "PRESS V TO START", olc::WHITE, 2);
			if (IsFocused()) {
				if (GetKey(olc::Key::V).bPressed) theGameBegins = true;
				bPlayerAAlive = true;
				bPlayerBAlive = true;
			}
			return true;
		}

		if (wireframeMode) {
			for (int x = 0; x < nVisibleTilesX + 1; x++) {
				for (int y = 0; y < nVisibleTilesY + 1; y++) {
					DrawRect(x * nTileWidth - fTileOffsetX, y * nTileHeight - fTileOffsetY, nTileWidth, nTileHeight, olc::WHITE);
				}
			}
		}
		
		// NOTE : for the sprite flip parameter,
		//	1 - horizontal | 2 - upsidedown | 3 - upsidedown and horizontal flip
		// drawing characters
		drawDeadPlayers(vMiddleSceneVolcanoLevelDeadPlayers, playerADead, playerBDead, fOffsetX, fOffsetY, fElapsedTime);
		drawAlivePlayers(bPlayerAAlive, fPlayerARespawn, fElapsedTime, fSwordPointAX, fSwordPointAY, fPlayerAPosX, fPlayerAPosY, fOffsetX, fOffsetY, nTileWidth, playerA, aReadySword, angledSword, iPlayerADirection, iSwordStatePlayerA, bLungingA, debugMode, true);
		drawAlivePlayers(bPlayerBAlive, fPlayerBRespawn, fElapsedTime, fSwordPointBX, fSwordPointBY, fPlayerBPosX, fPlayerBPosY, fOffsetX, fOffsetY, nTileWidth, playerB, aReadySword, angledSword, iPlayerBDirection, iSwordStatePlayerB, bLungingB, debugMode, false);

		return true;
	}
};

int main()
{
	Engine app;
	if (app.Construct(1920 / olcPixelDimen, 1080 / olcPixelDimen, olcPixelDimen, olcPixelDimen, true, false))
		app.Start();

	return 0;
}