#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"


// Override base class with your custom functionality
class AsteroidsGame : public olc::PixelGameEngine {

public: 
	
	AsteroidsGame() {
		sAppName = "Asteroids";
	}

private:

	struct sSpaceObject {
		int nSize;
		float x;
		float y;
		float dx;
		float dy;
		float angle;
	};

	vector<sSpaceObject> vecAsteroids;
	vector<sSpaceObject> vecBullets;
	sSpaceObject player;
	bool bDead = false;
	int nScore = 0;

	vector<pair<float, float>> vecModelShip;
	vector<pair<float, float>> vecModelAsteroid;

	bool debug = false;

	static const int MENU_STATE = 0;
	static const int GAME_STATE = 1;
	static const int PAUSE_MENU_STATE = 2;
	int currentState = 0;

protected:

	virtual bool OnUserCreate()
	{
		vecModelShip = {
			{  0.0f, -30.0f},
			{-20.0f, +20.0f},
			{+20.0f, +20.0f}
		}; // A simple Isoceles Triangle

		// Create a "jagged" circle for the asteroid. It's important it remains
		// mostly circular, as we do a simple collision check against a perfect
		// circle.
		int verts = 20;
		for (int i = 0; i < verts; i++)
		{
			float noise = (float)rand() / (float)RAND_MAX * 4.0f + 8.0f;
			vecModelAsteroid.push_back(
				make_pair(noise * sinf(((float)i / (float)verts) * 6.283185),
						  noise * cosf(((float)i / (float)verts) * 6.283185)));
		}

		ResetGame();
		return true;
	}

	float randFloat(float a, float b) {
		return rand() / RAND_MAX * (a - b) + b;
	}

	float randInt(int a, int b) {
		return (int) (rand() / RAND_MAX * (a - b) + b);
	}

	void ResetGame()
	{
		// Initialise Player Position
		player.x = ScreenWidth() / 2.0f;
		player.y = ScreenHeight() / 2.0f;
		player.dx = 0.0f;
		player.dy = 0.0f;
		player.angle = 0.0f;

		vecBullets.clear();
		vecAsteroids.clear();

		// Put in two asteroids
		vecAsteroids.push_back({ 16, randInt(0, 1) * 3 * ScreenWidth()/4.0f, ScreenHeight()/4.0f, 12.0f, -6.0f, 0.0f });
		vecAsteroids.push_back({ 16, randInt(0, 1) * 3 * ScreenWidth()/4.0f, ScreenHeight()/4.0f * 3.0f, -8.0f, 3.0f, 0.0f });

		// Reset game
		bDead = false;
		nScore = false;
	}

	void WrapCoordinates(float ix, float iy, float& ox, float& oy)
	{
		ox = ix;
		oy = iy;
		if (ix < 0.0f)	
			ox = ix + (float) ScreenWidth();
		else if (ix >= (float) ScreenWidth())
			ox = ix - (float) ScreenWidth();
		
		if (iy < 0.0f)	
			oy = iy + (float) ScreenHeight();
		else if (iy >= (float) ScreenHeight())
			oy = iy - (float) ScreenHeight();
	}

	bool IsPointInsideCircle(float cx, float cy, float radius, float x, float y)
	{
		return sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy)) < radius;
	}

	bool gameUpdate(float fElapsedTime) {

		if (bDead)
			ResetGame();

		// Clear Screen
		FillRect(0, 0, ScreenWidth(), ScreenHeight(), olc::Pixel(0, 0, 0));

		// Paused?
		if (GetKey(olc::Key::ESCAPE).bPressed) {
			currentState = PAUSE_MENU_STATE;
			return true;
		}

		// Steer Ship
		if (GetKey(olc::Key::LEFT).bHeld)
			player.angle -= 5.0f * fElapsedTime;
		if (GetKey(olc::Key::RIGHT).bHeld)
			player.angle += 5.0f * fElapsedTime;

		// Thrust? Apply ACCELERATION
		float acc_x = 0, acc_y = 0;
		if (GetKey(olc::Key::UP).bHeld)
		{
			// ACCELERATION changes VELOCITY (with respect to time)
			acc_x = sin(player.angle) * 100.0f;
			acc_y = -cos(player.angle) * 100.0f;
			player.dx += acc_x * fElapsedTime;
			player.dy += acc_y * fElapsedTime;
		}
		// v = u + at
		// s = vt - 0.5 * at^2
		// VELOCITY changes POSITION (with respect to time)
		player.x += player.dx * fElapsedTime - 0.5f * acc_x * fElapsedTime * fElapsedTime;
		player.y += player.dy * fElapsedTime - 0.5f * acc_y * fElapsedTime * fElapsedTime;

		// Keep ship in gamespace
		WrapCoordinates(player.x, player.y, player.x, player.y);

		// Check ship collision with asteroids
		for (auto& a : vecAsteroids)
			if (IsPointInsideCircle(a.x, a.y, a.nSize * 12, player.x, player.y))
				bDead = true; // Uh oh...

		// Fire Bullet in direction of player
		if (GetKey(olc::Key::SPACE).bPressed)
			vecBullets.push_back({ 4, player.x, player.y, 750.0f * sinf(player.angle), -750.0f * cosf(player.angle), 100.0f });

		// Update and draw asteroids
		for (auto& a : vecAsteroids)
		{
			// VELOCITY changes POSITION (with respect to time)
			a.x += a.dx * fElapsedTime;
			a.y += a.dy * fElapsedTime;
			a.angle += 0.5f * fElapsedTime; // Add swanky rotation :)

			// Asteroid coordinates are kept in game space (toroidal mapping)
			WrapCoordinates(a.x, a.y, a.x, a.y);

			// Draw Asteroids
			DrawWireFrameModel(vecModelAsteroid, a.x, a.y, a.angle, (float)a.nSize, olc::YELLOW);
		}

		// Any new asteroids created after collision detection are stored
		// in a temporary vector, so we don't interfere with the asteroids
		// vector iterator in the for(auto)
		vector<sSpaceObject> newAsteroids;

		// Update Bullets
		for (auto& b : vecBullets)
		{
			b.x += b.dx * fElapsedTime;
			b.y += b.dy * fElapsedTime;
			//WrapCoordinates(b.x, b.y, b.x, b.y);
			b.angle -= 1.0f * fElapsedTime;

			// Check collision with asteroids
			for (auto& a : vecAsteroids)
			{
				if (debug) {
					// Draw collision circle
					DrawCircle(a.x, a.y, a.nSize * 10, olc::Pixel(255, 255, 255));
				}

				//if (IsPointInsideRectangle(a.x, a.y, a.x + a.nSize, a.y + a.nSize, b.x, b.y))
				if (IsPointInsideCircle(a.x, a.y, a.nSize * 12, b.x, b.y))
				{
					// Asteroid Hit - Remove bullet
					// We've already updated the bullets, so force bullet to be offscreen
					// so it is cleaned up by the removal algorithm. 
					b.x = -100;

					// Create child asteroids
					if (a.nSize > 4)
					{
						float angle1 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
						float angle2 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
						newAsteroids.push_back({ (int)a.nSize >> 1 ,a.x + a.nSize * 3, a.y + a.nSize * 3, 20.0f * sinf(angle1), 20.0f * cosf(angle1), randFloat(0.0f, 6.283185f) });
						newAsteroids.push_back({ (int)a.nSize >> 1 ,a.x - a.nSize * 3, a.y - a.nSize * 3, 20.0f * sinf(angle2), 20.0f * cosf(angle2), randFloat(0.0f, 6.283185f) });
					}

					// Remove asteroid
					a.x = -100;
					nScore += 100; // Small score increase for hitting asteroid
				}
			}

		}

		// Append new asteroids to existing vector
		for (auto a : newAsteroids)
			vecAsteroids.push_back(a);

		// Clear up dead objects - They are out of game space

		// Remove asteroids that have been blown up
		if (vecAsteroids.size() > 0)
		{
			auto i = remove_if(vecAsteroids.begin(), vecAsteroids.end(), [&](sSpaceObject o) { return (o.x < 0); });
			if (i != vecAsteroids.end())
				vecAsteroids.erase(i);
		}

		if (vecAsteroids.empty()) // If no asteroids, level complete! :) - you win MORE asteroids!
		{
			// Level Clear
			nScore += 1000; // Large score for level progression
			vecAsteroids.clear();
			vecBullets.clear();

			// Add two new asteroids, but in a place where the player is not, we'll simply
			// add them 90 degrees left and right to the player, their coordinates will
			// be wrapped by th enext asteroid update
			vecAsteroids.push_back({ (int)16, 300.0f * sinf(player.angle - 3.14159f / 2.0f) + player.x,
											  300.0f * cosf(player.angle - 3.14159f / 2.0f) + player.y,
											  25.0f * sinf(player.angle), 25.0f * cosf(player.angle), 0.0f });

			vecAsteroids.push_back({ (int)16, 300.0f * sinf(player.angle + 3.14159f / 2.0f) + player.x,
											  300.0f * cosf(player.angle + 3.14159f / 2.0f) + player.y,
											  25.0f * sinf(-player.angle), 25.0f * cosf(-player.angle), 0.0f });

		}

		// Remove bullets that have gone off screen
		if (vecBullets.size() > 0)
		{
			auto i = remove_if(vecBullets.begin(), vecBullets.end(), [&](sSpaceObject o) { return (o.x < 1 || o.y < 1 || o.x >= ScreenWidth() - 1 || o.y >= ScreenHeight() - 1); });
			if (i != vecBullets.end())
				vecBullets.erase(i);
		}

		// Draw Bullets
		for (auto b : vecBullets)
			FillCircle(b.x, b.y, 4);

		// Draw Ship
		DrawWireFrameModel(vecModelShip, player.x, player.y, player.angle);


		// Draw Score
		DrawString(2, 2, ("SCORE: " + to_string(nScore)));
		return true;
	}

	bool menuUpdate(float fElapsedTime) {
		DrawStringDecal({ 200, 100 }, "ASTEROIDS", { 255, 255, 255 }, { 8.0f, 8.0f });

		DrawStringDecal({ 100, 300 }, "> Press ENTER to start <", { 255, 255, 255 }, { 4.0f, 4.0f });

		if (GetKey(olc::Key::ENTER).bPressed) currentState = GAME_STATE;

		return true;
	}

	bool pauseUpdate(float fElapsedTime) {
		DrawStringDecal({ 200, 100 }, "PAUSED", { 255, 255, 255 }, { 5.0f, 5.0f });

		DrawStringDecal({ 100, 200 }, "> Press ENTER to resume <", { 255, 255, 255 }, { 1.5f, 1.5f });
		DrawStringDecal({ 100, 230 }, "> Press R to restart <", { 255, 255, 255 }, { 1.5f, 1.5f });

		if (GetKey(olc::Key::R).bPressed) {
			currentState = GAME_STATE;
			ResetGame();
		}
		else if (GetKey(olc::Key::ENTER).bPressed) currentState = GAME_STATE;

		return true;
	}

	virtual bool OnUserUpdate(float fElapsedTime)
	{
		switch (currentState)
		{
		case MENU_STATE: return menuUpdate(fElapsedTime);
		case GAME_STATE: return gameUpdate(fElapsedTime);
		case PAUSE_MENU_STATE: return pauseUpdate(fElapsedTime);
		default: return true;
		}

	}

	void DrawWireFrameModel(const vector<pair<float, float>>& vecModelCoordinates, float x, float y, float r = 0.0f, float s = 1.0f, olc::Pixel col = olc::WHITE)
	{
		// pair.first = x coordinate
		// pair.second = y coordinate

		// Create translated model vector of coordinate pairs
		vector<pair<float, float>> vecTransformedCoordinates;
		int verts = vecModelCoordinates.size();
		vecTransformedCoordinates.resize(verts);

		// Rotate
		for (int i = 0; i < verts; i++)
		{
			vecTransformedCoordinates[i].first = vecModelCoordinates[i].first * cosf(r) - vecModelCoordinates[i].second * sinf(r);
			vecTransformedCoordinates[i].second = vecModelCoordinates[i].first * sinf(r) + vecModelCoordinates[i].second * cosf(r);
		}

		// Scale
		for (int i = 0; i < verts; i++)
		{
			vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first * s;
			vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second * s;
		}

		// Translate
		for (int i = 0; i < verts; i++)
		{
			vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first + x;
			vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second + y;
		}

		// Draw Closed Polygon
		for (int i = 0; i < verts + 1; i++)
		{
			int j = (i + 1);
			DrawLine(vecTransformedCoordinates[i % verts].first, vecTransformedCoordinates[i % verts].second,
				vecTransformedCoordinates[j % verts].first, vecTransformedCoordinates[j % verts].second, col);
		}
	}
};


int main()
{
	// Use olcConsoleGameEngine derived app
	AsteroidsGame game;
	game.Construct(960, 540, 1, 1);
	game.Start();
	return 0;
}