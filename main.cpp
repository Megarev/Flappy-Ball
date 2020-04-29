#include <SFML/Graphics.hpp>
#include "AssetManager.h"
#include "GraphicsRender.h"
#include "GraphicsUI.h"
#include <iostream>
#include <ctime>

class GameState {
private:
	void LoadAssets() {
		AssetHolder::Get().AddFont("sansationBold", "files/fonts/Sansation_Bold.ttf");
		AssetHolder::Get().AddTexture("gameTitle", "files/images/gameTitle.png");
		AssetHolder::Get().AddTexture("background", "files/images/background.png");
	}
public:
	sf::Vector2u windowSize = { 800, 600 };

	enum {
		Menu = 0,
		Play = 1,
		HighScore = 2,
		Quit = 3
	} state;
	bool isStateChanged;

	GameState() {
		isStateChanged = false;
		LoadAssets();
	}
	virtual void Input() {}
	virtual void ManageEvent(sf::Event, sf::Vector2f) {}
	virtual void Logic(float) = 0;
	virtual void Render(sf::RenderWindow&) = 0;

	void LoadScores(const std::string& filepath, std::vector<int>& scores) {
		std::ifstream reader(filepath);

		if (reader.is_open()) {

			while (!reader.eof()) {
				int value;
				reader >> value;
				scores.push_back(value);
			}

			reader.close();
		}
	}

	void SaveScores(const std::string& filepath, std::vector<int>& scores) {
		std::ofstream writer(filepath);

		std::sort(scores.rbegin(), scores.rend());

		if (writer.is_open()) {

			for (std::size_t i = 0; i < (scores.size() <= 10 ? scores.size() : 10); i++) {
				writer << scores[i] << "\n";
			}

			writer.close();
		}
	}

	static bool KeyPress(sf::Keyboard::Key key) {
		return sf::Keyboard::isKeyPressed(key);
	};

	static bool MousePress(sf::Mouse::Button button) {
		return sf::Mouse::isButtonPressed(button);
	};
};

class Pipe {
private:
	sf::Vector2f position, pipeSize;
	sf::FloatRect bounds;
	float speed;
public:
	static const int distancePipesY = 125;
	Pipe() {}
	Pipe(const sf::Vector2f& pos, const sf::Vector2f& size)
		: position(pos), pipeSize(size) {
		speed = 100.0f;

		bounds = sf::FloatRect(pos, size);
	}

	void ResetPipe(const sf::Vector2f& pos, const sf::Vector2f& size) {
		position = pos;
		pipeSize = size;

		bounds = sf::FloatRect(pos, size);
	}

	inline sf::Vector2f GetPosition() const { return position; }
	void SetPosition(const sf::Vector2f& pos) { position = pos; }

	inline sf::FloatRect GetBounds() const { return bounds; }

	inline sf::Vector2f GetSize() const { return pipeSize; }
	void SetSize(const sf::Vector2f& size) { pipeSize = size; }

	void Logic(const sf::Vector2u windowSize, float dt) {
		position.x -= speed * dt;
		bounds = sf::FloatRect(position, pipeSize);

		if (position.x + pipeSize.x < 0.0f) {
			position.x = windowSize.x;
		}
	}

	static std::pair<float, float> CalculateHeights(float minHeight, float maxHeight, const sf::Vector2u& windowSize) {
		std::pair<float, float> heights;

		heights.first = (rand() % (int)maxHeight) + minHeight;
		heights.second = std::fabs(windowSize.y - (heights.first + (float)distancePipesY));

		return heights;
	}

	void Render(sf::RenderWindow& window, sf::RectangleShape& box) {
		box.setSize(pipeSize);
		box.setPosition(position);
		window.draw(box);
	}
};

class MenuState : public GameState {
private:
	sf::Sprite background, gameTitle;
	std::vector<Button> buttons;
	sf::Vector2f buttonSize;
	const std::string buttonNames[3] = { "Play", "Scores", "Quit" };

	void Initialize() {
		for (std::size_t i = 0; i < 3; i++) {
			buttons.push_back(Button());
			buttons[i].Initialize({ (windowSize.x - buttonSize.x) / 2.0f, (windowSize.y / 2.0f) + 75.0f + i * (buttonSize.y + 10.0f) }, buttonSize);
			sf::Color randColor = sf::Color(rand() % 100, rand() % 100, rand() % 100);
			buttons[i].SetColors(sf::Color(randColor.r + 100, randColor.g + 100, randColor.b + 100),
				sf::Color(randColor.r + 50, randColor.g + 50, randColor.b + 50), randColor);
			buttons[i].SetOutline(-5.0f, sf::Color(randColor.r + 25, randColor.g + 25, randColor.b + 25));

			buttons[i].ResetColor();
		}
	}
public:
	MenuState() {
		background.setTexture(AssetHolder::Get().GetTexture("background"));
		gameTitle.setTexture(AssetHolder::Get().GetTexture("gameTitle"));
		gameTitle.setPosition({ 150.0f, 0.0f });

		buttonSize = { 128.0f, 40.0f };

		Initialize();
	}

	void ManageEvent(sf::Event e, sf::Vector2f mousePos) override {
		for (auto& button : buttons) {
			button.Logic(e, mousePos);
		}

		switch (e.type) {
		case sf::Event::MouseButtonPressed:
			switch (e.key.code) {
			case sf::Mouse::Left:
				for (std::size_t i = 0; i < buttons.size(); i++) {
					if (buttons[i].IsPositionInBounds(mousePos)) {
						switch (i + 1) {
						case Play:
							state = Play;
							break;
						case HighScore:
							state = HighScore;
							break;
						case Quit:
							state = Quit;
							break;
						}

						isStateChanged = true;
					}
				}
				break;
			}
			break;
		}
	}

	void Logic(float dt) override {}

	void Render(sf::RenderWindow& window) override {

		window.draw(background);
		window.draw(gameTitle);

		for (std::size_t i = 0; i < buttons.size(); i++) {
			buttons[i].Render(window);
			RenderText(window, AssetHolder::Get().GetFont("sansationBold"), i == 1 ? buttons[i].GetPosition().x + 15.0f : buttons[i].GetPosition().x + 30.0f, buttons[i].GetPosition().y, buttonNames[i]);
		}
	}
};

class HighScoreState : public GameState {
private:
	std::vector<int> scores;
	sf::Sprite background;

	sf::Color randColor;
	float lineThickness;
public:
	HighScoreState() {
		LoadScores("files/scores.txt", scores);

		randColor = sf::Color(rand() % 256, rand() % 256, rand() % 256);

		lineThickness = 4.0f;
		background.setTexture(AssetHolder::Get().GetTexture("background"));
	}

	void ManageEvent(sf::Event e, sf::Vector2f mousePos) override {
		switch (e.type) {
		case sf::Event::KeyPressed:
			switch (e.key.code) {
			case sf::Keyboard::Escape:
				state = Menu;
				isStateChanged = true;
				break;
			}
			break;
		}
	}

	void Logic(float dt) override {}
	void Render(sf::RenderWindow& window) override {

		window.draw(background);

		RenderText(window, AssetHolder::Get().GetFont("sansationBold"), (windowSize.x / 2.0f) - 70.0f, 0.0f,
			"Scores", randColor, 52);
		for (float i = 0; i < lineThickness; i++) {
			DrawLine(window, (windowSize.x / 2.0f) - 100.0f, 55.0f + i, (windowSize.x / 2.0f) + 125.0f, 55.0f + i, randColor);
		}
		for (std::size_t i = 0; i < scores.size(); i++) {
			RenderText(window, AssetHolder::Get().GetFont("sansationBold"), 25.0f, 164.0f + i * 32.0f, "-> " + std::to_string(scores[i]));
		}
	}
};

class PlayState : public GameState {
private:
	sf::CircleShape circle;
	sf::RectangleShape box;
	sf::Sprite background;
	sf::Vector2f velocity;
	float jumpSpeed, gSpeed, gMax;
	sf::Vector2u initWindowSize;
	int score;
	bool isHit;
	std::vector<int> scores;

	struct PipePair {
		Pipe pipes[2];
		sf::FloatRect rectBetweenPipes;
		float speed = 100.0f;

		void Initialize(const sf::Vector2f& position, const sf::Vector2u& windowSize) {
			auto heights = Pipe::CalculateHeights(75.0f, (windowSize.y - 275.0f), windowSize);
			pipes[0] = Pipe(position, { 50.0f, heights.first });
			pipes[1] = Pipe({ position.x, position.y + Pipe::distancePipesY + heights.first }, { 50.0f, heights.second });

			rectBetweenPipes = sf::FloatRect(position.x, position.y + heights.first, 50.0f, Pipe::distancePipesY);
		}

		bool IsIntersectBounds(sf::CircleShape& circle) {
			for (std::size_t i = 0; i < 2; i++) {
				if (pipes[i].GetBounds().intersects(circle.getGlobalBounds())) {
					return true;
				}
			}

			return false;
		}

		bool isCircleBetweenPipes(sf::CircleShape& circle) {
			return rectBetweenPipes.intersects(circle.getGlobalBounds());
		}

		void Logic(const sf::Vector2u& windowSize, float dt) {
			for (std::size_t i = 0; i < 2; i++) {
				pipes[i].Logic(windowSize, dt);
			}

			rectBetweenPipes.left -= speed * dt;
			if (rectBetweenPipes.left + rectBetweenPipes.width < 0.0f) {
				rectBetweenPipes.left = windowSize.x;

				Initialize({ (float)windowSize.x, 0.0f }, windowSize);
			}
		}

		void Render(sf::RenderWindow& window, sf::RectangleShape& box) {
			for (std::size_t i = 0; i < 2; i++) {
				pipes[i].Render(window, box);
			}
		}
	};

	const int n = 5;
	std::vector<PipePair> pipePairs;
public:
	PlayState() {

		LoadScores("files/scores.txt", scores);

		circle.setRadius(16.0f);
		circle.setPosition({ 25.0f, 25.0f });
		circle.setFillColor(sf::Color(rand() % 256, rand() % 256, rand() % 256));
		box.setFillColor(sf::Color(rand() % 256, rand() % 256, rand() % 256));

		initWindowSize = { 800, 600 };

		background.setTexture(AssetHolder::Get().GetTexture("background"));

		isHit = false;

		score = 0;
		gSpeed = 50.0f;
		jumpSpeed = 350.0f;
		gMax = 400.0f;

		for (int i = 0; i < n; i++) {
			pipePairs.push_back(PipePair());
			pipePairs[i].Initialize({ 200.0f + windowSize.x - i * 168.0f, 0.0f }, windowSize);
		}
	}

	void Input() override {
		if ((KeyPress(sf::Keyboard::Space) || MousePress(sf::Mouse::Left)) && !isHit) {
			velocity.y = -jumpSpeed;
		}
	}

	void ManageEvent(sf::Event e, sf::Vector2f mousePos) {
		switch (e.type) {
		case sf::Event::KeyPressed:
			switch (e.key.code) {
			case sf::Keyboard::Escape:
				state = Menu;
				isStateChanged = true;
				break;
			}
			break;
		}
	}

	void Logic(float dt) override {

		velocity.y += gSpeed;
		if (velocity.y > gMax) velocity.y = gMax;

		if (!isHit) {
			for (auto& pipes : pipePairs) {
				pipes.Logic(windowSize, dt);
				if (pipes.IsIntersectBounds(circle)) {
					isHit = true;
				}
				else if (pipes.isCircleBetweenPipes(circle)) {
					if (circle.getPosition().x + 1.75f > pipes.rectBetweenPipes.left + pipes.rectBetweenPipes.width) {
						score++;
					}
				}
			}
		}

		circle.move({ 0.0f, velocity.y * dt });
		if (circle.getPosition().y < 0.0f) {
			circle.setPosition({ circle.getPosition().x, 0.0f });
		}

		if (circle.getPosition().y > windowSize.y) {
			sf::sleep(sf::seconds(1.0f));
			scores.push_back(score);
			SaveScores("files/scores.txt", scores);
			state = Menu;
			isStateChanged = true;
		}
	}

	void Render(sf::RenderWindow& window) override {

		window.draw(background);

		window.draw(circle);
		for (auto& pipes : pipePairs) {
			pipes.Render(window, box);
		}

		DrawTextWithValue(window, AssetHolder::Get().GetFont("sansationBold"), 0.0f, 0.0f, "Score : ", score, sf::Color::Yellow);
	}
};

class Game {
private:
	sf::RenderWindow Window;
	sf::Vector2u windowSize;
	std::string windowTitle;

	std::unique_ptr<GameState> gameState;

	sf::Clock clock;
	float initDt;
public:
	Game(const sf::Vector2u& size, const std::string& title)
		: windowSize(size), windowTitle(title),
		Window({ size.x, size.y }, title, sf::Style::Titlebar | sf::Style::Close) {
		Window.setFramerateLimit(60);

		gameState = std::make_unique<MenuState>();

		initDt = 0.0f;
	}

	void Logic() {
		while (Window.isOpen()) {

			if (gameState->isStateChanged) {
				switch (gameState->state) {
				case GameState::Menu:
					gameState = std::make_unique<MenuState>();
					break;
				case GameState::Play:
					gameState = std::make_unique<PlayState>();
					break;
				case GameState::HighScore:
					gameState = std::make_unique<HighScoreState>();
					break;
				case GameState::Quit:
					Window.close();
					break;
				}
				gameState->isStateChanged = false;
			}

			sf::Event event;

			float newDt = (float)clock.getElapsedTime().asSeconds();
			float dt = newDt - initDt;
			initDt = newDt;
			if (newDt > 0.25f) newDt = 0.25f;

			sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(Window);

			while (Window.pollEvent(event)) {
				switch (event.type) {
				case sf::Event::Closed:
					Window.close();
					break;
				case sf::Event::Resized:
					windowSize = { (uint32_t)event.size.width, (uint32_t)event.size.height };
					Window.create({ windowSize.x, windowSize.y }, windowTitle);
					gameState->windowSize = windowSize;
					break;
				}

				gameState->ManageEvent(event, mousePos);
			}

			gameState->Input();
			gameState->Logic(dt);

			Window.clear();
			gameState->Render(Window);
			Window.display();
		}
	}

	void Run() {
		Logic();
	}
};

int main() {

	srand((unsigned)time(0));

	Game game({ 800, 600 }, "Game");
	game.Run();

	return 0;
}