#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>
#include <random>
#include <iostream>

constexpr float kSlowStep = 1. / 60.;
constexpr float kFastStep = 1. / 240.;
constexpr float kResilience = 1.;

enum Mode {CCD, MR};
struct Body {
  sf::Vector2f position;
  sf::Vector2f velocity;
  sf::Vector2f half_size;
  float mass;  // 0 - стена
};

Body makeBody(sf::Vector2f position, sf::Vector2f velocity,
                     sf::Vector2f half_size) {
  return {position, velocity, half_size, 1.f / (half_size.x * half_size.y)};
}

using Group = std::vector<Body>;

void addImpact(Body& a, Body& b, sf::Vector2f normal) {
  const sf::Vector2f velocity = b.velocity - a.velocity;
  const float normal_speed = velocity.x * normal.x + velocity.y * normal.y;
  if (normal_speed >= 0.) return;
  const float impulse = -(1.0f + kResilience) * normal_speed / (a.mass + b.mass);
  a.velocity -= impulse * a.mass * normal;
  b.velocity += impulse * b.mass * normal;
}

void advance(Body& body, float dt) {
  body.position += body.velocity * dt;
}

void resolveCollision(Body& a, Body& b) {
  const sf::Vector2f offset = b.position - a.position;
  const sf::Vector2f overlap = a.half_size + b.half_size - sf::Vector2f(std::abs(offset.x), std::abs(offset.y));
  if (overlap.x <= 0. || overlap.y <= 0.) return;

  sf::Vector2f normal;
  float depth;
  if (overlap.x < overlap.y) {
    normal = {offset.x > 0.f ? 1.f : -1.f, 0.f};
    depth = overlap.x;
  } else {
    normal = {0.f, offset.y > 0.f ? 1.f : -1.f};
    depth = overlap.y;
  }

  const float total_mass = a.mass + b.mass;
  a.position -= normal * depth * (a.mass / total_mass);
  b.position += normal * depth * (b.mass / total_mass);

  addImpact(a, b, normal);
}

void checkCollisions(Body& body, const std::vector<Body*> all_bodies, float dt) {
  advance(body, dt);
    for (auto other : all_bodies) {
      if (other != &body) 
        resolveCollision(body, *other);
  }
}

bool findTimeOfImpact(const Body* a, const Body* b, float max_time,
                             float& time, sf::Vector2f& normal) {
  const sf::Vector2f velocity = a->velocity - b->velocity;
  const sf::Vector2f offset = a->position - b->position;
  const sf::Vector2f extent = a->half_size + b->half_size;

  const float tx1 = (-extent.x - offset.x) / velocity.x;
  const float tx2 = (extent.x - offset.x) / velocity.x;
  const float ty1 = (-extent.y - offset.y) / velocity.y;
  const float ty2 = (extent.y - offset.y) / velocity.y;
  const float entry_x = std::min(tx1, tx2);
  const float entry_y = std::min(ty1, ty2);
  const float entry = std::max(entry_x, entry_y);
  const float exit = std::min(std::max(tx1, tx2), std::max(ty1, ty2));

  if (entry < 0.0f || entry > exit || entry > max_time || entry != entry || exit != exit) return false;

  time = entry;
  if (entry_x > entry_y) normal = {velocity.x > 0.0f ? 1.0f : -1.0f, 0.0f};
  else normal = {0.0f, velocity.y > 0.0f ? 1.0f : -1.0f};

  return true;
}

void stepCCD(std::vector<Body*>& bodies, float dt) {
  float remaining = dt;
  while(true) {
    float earliest_time = remaining;
    sf::Vector2f earliest_normal;
    int first = 0;
    int second = 0;
    bool found = false;

    for (int a = 0; a < bodies.size(); ++a) {
      for (int b = a + 1; b < bodies.size(); ++b) {
        if (bodies[a]->mass + bodies[b]->mass == 0.) continue;

        float time;
        sf::Vector2f normal;
        if (findTimeOfImpact(bodies[a], bodies[b], earliest_time, time, normal)) {
          earliest_time = time;
          earliest_normal = normal;
          first = a;
          second = b;
          found = true;
        }
      }
    }

    if (!found) break;
    for (auto body: bodies) advance(*body, earliest_time);
    remaining -= earliest_time;
    addImpact(*bodies[first], *bodies[second], earliest_normal);
  }

  for (auto body : bodies) advance(*body, remaining);
}

void draw(sf::RenderWindow& window, const Group& bodies, sf::Color color) {
  for (const Body& body : bodies) {
    sf::RectangleShape shape(body.half_size * 2.f);
    shape.setOrigin(body.half_size);
    shape.setPosition(body.position);
    shape.setFillColor(color);
    window.draw(shape);
  }
}

int main() {
  sf::RenderWindow window(sf::VideoMode({1280u, 720u}), "CCD");
  Mode mode = Mode::CCD;

  // create test

  std::mt19937 engine(std::random_device{}());
  std::uniform_real_distribution<float> half_size_range(5., 10.);
  std::uniform_real_distribution<float> speed_range(-150., 150.);
  Group fasts;
  Group slows;
  Group statics = {{{50., 360.}, {0., 0.}, {5., 200.0}, 0.},
                   {{640., 560.}, {0., 0.}, {600, 5.}, 0.},
                   {{640, 160.}, {0., 0.}, {600, 5.}, 0.},
                   {{1230., 360.}, {0., 0.}, {5., 200.}, 0.}};

  for (int i = 0; i < 5; ++i) statics.push_back( {{300.f + 200.f * i, 250.f}, {0.f, 0.f}, {20., 20.}, 0.f});
  
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 10; ++j)
      slows.push_back(makeBody(
          {65.f + 100.f * j, 375.f + 30.f * i},
          {speed_range(engine), speed_range(engine)},
          {half_size_range(engine), half_size_range(engine)}));

   for (int i = 0; i < 5; ++i) 
     fasts.push_back(makeBody({100.f, 300.f + 30.f * i}, {1200.f, 400.f - 200.f * i}, {3., 3.}));


  //for (int i = 0; i < 70; ++i) 
  //  fasts.push_back(makeBody({100.f + 10.f * i, 360.f}, {500.f, 0.f}, {5.f, 5.f}));

  //  end create test

  sf::Clock clock;
  float slow_timer = 0.f;
  float fast_timer = 0.f;
  const std::vector<Group*> groups = {&statics, &slows, &fasts};
  std::vector<Body*> all_bodies;
  for (Group* group : groups) 
    for (Body& body : *group) all_bodies.push_back(&body);

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) window.close();

      const auto* key = event->getIf<sf::Event::KeyPressed>();
      if (key != nullptr && key->code == sf::Keyboard::Key::Space) {
        mode = Mode(1 ^ mode);
        window.setTitle(mode == Mode::CCD ? "CCD 60 Hz"  : "Multi-rate 60/240 Hz");
      }
    }

    const float dt = clock.restart().asSeconds();
    slow_timer += dt;
    while (slow_timer >= kSlowStep) {
      if (mode == Mode::CCD) stepCCD(all_bodies, kSlowStep);
      else for (Body& body : slows) checkCollisions(body, all_bodies, kSlowStep);

      slow_timer -= kSlowStep;
    }

    if (mode == Mode::CCD) fast_timer = 0.;
    else {
      fast_timer += dt;
      while (fast_timer >= kFastStep) {
        for (Body& body : fasts) checkCollisions(body, all_bodies, kFastStep);
        fast_timer -= kFastStep;
      }
    }

    window.clear();
    draw(window, statics, mode == Mode::CCD ? sf::Color::Green : sf::Color::Yellow);
    draw(window, slows, sf::Color::Blue);
    draw(window, fasts, sf::Color::Red);
    // std::cout << mode << ' ' << (clock.getElapsedTime().asSeconds()) << '\n';
    window.display();
  }
}