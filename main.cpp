#include <raylib.h>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <string>
#include <cmath>
#include <list>

constexpr int window_size = 1080;
constexpr float max_node_size = 500.0f;
constexpr float min_node_size = 150.0f;

constexpr float spread_multiplier = 100.0f;
constexpr float gravity_multiplier = 1.1f;
constexpr float force_multiplier = 4000.0f;

class Node {
    public:
        unsigned long size;
        const std::string name;
        float circleSize;
        bool pushed;
        Vector2 pos;
        Vector2 force;
        std::list<Node> children;
    public:
        Node(const unsigned long size, const std::string& name)
            :
            size(size),
            name(name),
            circleSize(1.0f),
            pushed(false),
            pos(Vector2{
                static_cast<float>(rand() % static_cast<int>((2.0f * spread_multiplier * window_size) - (spread_multiplier * window_size))),
                static_cast<float>(rand() % static_cast<int>((2.0f * spread_multiplier * window_size) - (spread_multiplier * window_size)))
            }),
            force(Vector2{ 0.0f, 0.0f })
            {
            }

        float mass() const {
            return (2.0f * PI * circleSize) / 1.5f;
        }

        void update() {
            const Vector2 vel = Vector2{force.x / this->mass(), force.y / this->mass()};
            this->pos = Vector2{ this->pos.x + vel.x, this->pos.y + vel.y };
        }
};

void addNodes(Node& current, const std::string& path) {
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(path)) {
        if (is_directory(entry.path())) {
            const Node node(0, entry.path().filename());
            current.children.push_back(node);
            addNodes(current.children.back(), entry.path());
        } else {
            try {
                const Node node(entry.file_size(), entry.path().filename());
                current.children.push_front(node);
            } catch (const std::filesystem::filesystem_error& e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}

void getMaxSizes(Node& current, unsigned long& maxSize, unsigned long& minSize) {
    for (Node& node : current.children) {
        getMaxSizes(node, maxSize, minSize);
    }
    if (current.size < minSize) {
        minSize = current.size;
    }
    if (current.size > maxSize) {
        maxSize = current.size;
    }
}

unsigned long calculateFolderSizes(Node& current) {
    for (Node& node : current.children) {
        current.size += calculateFolderSizes(node);
    }
    return current.size;
}

void calculateCircleSizes(Node& current, unsigned long& maxSize, unsigned long& minSize) {
    for (Node& node : current.children) {
        calculateCircleSizes(node, maxSize, minSize);
    }
    current.circleSize = ((float)current.size - (float)minSize) * (max_node_size - min_node_size) / ((float)maxSize - (float)minSize) + min_node_size;
}

void drawNodes(Node& current, Vector2 parentPos, unsigned short depth) {
    current.pushed = false;
    DrawCircleV(current.pos, current.circleSize, Color{ 255, 255, 255, 255 });
    DrawText(current.name.c_str(), current.pos.x, current.pos.y, 3 * current.circleSize, Color{ 255, 255, 255, 255 });
    if (depth != 0) {
        DrawLineV(current.pos, parentPos, Color{ 255, 255, 255, 75 });
    }
    current.update();
    for (Node& node : current.children) {
        drawNodes(node, current.pos, depth + 1);
    }
}

void repulseFromOthers(Node& main, Node& current) {
    if (!current.pushed) {
        const Vector2 dir = Vector2{ current.pos.x - main.pos.x, current.pos.y - main.pos.y };
        const float magSq = std::sqrt(dir.x * dir.x + dir.y * dir.y) * 2.0f;
        const Vector2 force = Vector2 { dir.x / magSq * force_multiplier, dir.y / magSq * force_multiplier };
        main.force = Vector2{ main.force.x - force.x, main.force.y - force.y };
        current.force = Vector2{ current.force.x + force.x, current.force.y + force.y };
    }
    for (Node& node : current.children) {
        repulseFromOthers(main, node);
    }
}

void applyForces(Node& current, Node& root) {
    for (Node& node : current.children) {
        applyForces(node, root);
    }
    // Apply gravity force on node
    current.force = Vector2{ -current.pos.x * gravity_multiplier, -current.pos.y * gravity_multiplier };
    // Repulse from every other node
    current.pushed = true;
    repulseFromOthers(current, root);
    // Attract from connected nodes
    for (Node& node : current.children) {
        const Vector2 dis = Vector2{ current.pos.x - node.pos.x, current.pos.y - node.pos.y };
        current.force = Vector2{ current.force.x - dis.x, current.force.y - dis.y };
        node.force = Vector2{ node.force.x + dis.x, node.force.y + dis.y };
    }
}

void debugPrint(Node& current, unsigned short depth) {
    for (unsigned short i = 0; i < depth; ++i) {
        std::cout << "\t";
    }
    std::cout << current.name << ", " << current.size << std::endl;
    for (Node& node : current.children) {
        debugPrint(node, depth + 1);
    }
}

int main(int argc, char *argv[]) {
    // Initial setup
    srand(time(NULL));
    std::string pathMain = (std::string)getenv("HOME") + "/Documents/Progetti";
    if (argc > 1) {
        srand(strtol(argv[1], NULL, 10));
        if (argc > 2) {
            pathMain = argv[2];
            if (argc > 3) {
                std::cout << "Syntax error, correct usage:" << std::endl;
                std::cout << "graph" << std::endl;
                std::cout << "graph <seed>" << std::endl;
                std::cout << "graph <seed> <rootPath>" << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
    // Gather data
    Node root(0, "Root");
    addNodes(root, pathMain);
    calculateFolderSizes(root);
    unsigned long maxSize = 0;
    unsigned long minSize = -1;
    getMaxSizes(root, maxSize, minSize);
    calculateCircleSizes(root, maxSize, minSize);
    // Generating window and graph
    InitWindow(window_size, window_size, "Graph View");
    SetTargetFPS(60);
    Camera2D camera = { 0.0f };
    camera.target = Vector2{ static_cast<float>(-window_size) / 2.0f, static_cast<float>(-window_size) / 2.0f };
    camera.offset = Vector2{ static_cast<float>(window_size) / 2.0f, static_cast<float>(window_size) / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 0.1f;
    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_P)) {
            camera.zoom += 0.001f;
        }
        if (IsKeyDown(KEY_O)) {
            camera.zoom -= 0.001f;
        }
        if (IsKeyDown(KEY_UP)) {
            camera.target.y -= 1 / camera.zoom * 3.0f;
        }
        if (IsKeyDown(KEY_DOWN)) {
            camera.target.y += 1 / camera.zoom * 3.0f;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            camera.target.x += 1 / camera.zoom * 3.0f;
        }
        if (IsKeyDown(KEY_LEFT)) {
            camera.target.x -= 1 / camera.zoom * 3.0f;
        }
        applyForces(root, root);
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);
        drawNodes(root, root.pos, 0);
        EndMode2D();
        EndDrawing();
    }
    CloseWindow();
    return EXIT_SUCCESS;
}
