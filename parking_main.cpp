// AEON MALL Da Nang Thanh Khe - representative B1 parking simulation.
// C++17 / OpenGL 3.3 / GLFW / GLEW / GLM.
// All physical layout coordinates are generated from aeon_layout.h.
// Values labelled [ASSUMED] are design references, NOT surveyed AEON measurements.
//
// Object-oriented entry point: the Application class owns the window, the
// renderer, the camera, the simulation world and presents the two views
// (SceneRenderer for the 3D scene, Hud for the information panel).

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdio>
#include <cstddef>
#include <string>

#include "aeon_layout.h"
#include "camera.h"
#include "parking_world.h"
#include "renderer.h"
#include "scene_view.h"

namespace
{

    // Owns every subsystem and the GLFW window; drives the frame loop.
    class Application
    {
    public:
        int run();

    private:
        bool initialise();
        void close();
        void updateSimulation(float dt);
        CameraContext buildCameraContext(float dt) const;
        void renderFrame(int width, int height);
        void updateWindowTitle();

        void handleKey(int key);
        ParkingWorld &activeWorld() { return selectedFloor_ == AeonLayout::FLOOR_INDEX_B2 ? worldB2_ : worldB1_; }
        const ParkingWorld &activeWorld() const { return selectedFloor_ == AeonLayout::FLOOR_INDEX_B2 ? worldB2_ : worldB1_; }
        void handleMouseButton(int button, int action);
        void handleCursor(double x, double y);
        void handleScroll(double y);
        void syncCursorMode();

        static void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
        static void onMouseButton(GLFWwindow *window, int button, int action, int mods);
        static void onCursorPos(GLFWwindow *window, double x, double y);
        static void onScroll(GLFWwindow *window, double xOffset, double yOffset);

        GLFWwindow *window_ = nullptr;
        Renderer renderer_;
        Camera camera_;
        ParkingWorld worldB1_{AeonLayout::FLOOR_HEIGHT_B1_M, AeonLayout::FLOOR_INDEX_B1};
        ParkingWorld worldB2_{AeonLayout::FLOOR_HEIGHT_B2_M, AeonLayout::FLOOR_INDEX_B2};
        ElevatorSystem elevator_;
        int selectedFloor_ = AeonLayout::FLOOR_INDEX_B1;
        SceneRenderer scene_{renderer_};
        Hud hud_{renderer_};

        bool paused_ = false;
        bool dragging_ = false;
        bool cursorCaptured_ = false; // true while the cursor is hidden for free-look
        bool skipNextCursorDelta_ = false;
        double lastMouseX_ = 0.0;
        double lastMouseY_ = 0.0;
    };

    bool Application::initialise()
    {
        std::fprintf(stderr, "[init] glfwInit...\n");
        std::fflush(stderr);
        if (!glfwInit())
        {
            std::fprintf(stderr, "GLFW initialisation failed.\n");
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, AeonLayout::OPENGL_CONTEXT_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, AeonLayout::OPENGL_CONTEXT_MINOR);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, AeonLayout::MULTISAMPLE_SAMPLES);

        std::fprintf(stderr, "[init] glfwCreateWindow...\n");
        std::fflush(stderr);
        window_ = glfwCreateWindow(AeonLayout::WINDOW_WIDTH_PX, AeonLayout::WINDOW_HEIGHT_PX,
                                   "AEON MALL Da Nang Thanh Khe - B1 Zone A Parking Simulation",
                                   nullptr, nullptr);
        if (!window_)
        {
            std::fprintf(stderr, "Could not create the OpenGL window.\n");
            glfwTerminate();
            return false;
        }

        std::fprintf(stderr, "[init] glfwMakeContextCurrent...\n");
        std::fflush(stderr);
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);
        glewExperimental = GL_TRUE;
        std::fprintf(stderr, "[init] glewInit...\n");
        std::fflush(stderr);
        if (glewInit() != GLEW_OK)
        {
            std::fprintf(stderr, "GLEW initialisation failed.\n");
            glfwDestroyWindow(window_);
            glfwTerminate();
            window_ = nullptr;
            return false;
        }
        glGetError();

        glfwSetWindowUserPointer(window_, this);
        glfwSetKeyCallback(window_, &Application::onKey);
        glfwSetMouseButtonCallback(window_, &Application::onMouseButton);
        glfwSetCursorPosCallback(window_, &Application::onCursorPos);
        glfwSetScrollCallback(window_, &Application::onScroll);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);

        std::fprintf(stderr, "[init] renderer_.initialise()...\n");
        std::fflush(stderr);
        if (!renderer_.initialise())
        {
            close();
            return false;
        }
        std::fprintf(stderr, "[init] scene_.initialise()...\n");
        std::fflush(stderr);
        if (!scene_.initialise())
        {
            close();
            return false;
        }
        std::fprintf(stderr, "[init] done.\n");
        std::fflush(stderr);
        return true;
    }

    void Application::close()
    {
        renderer_.shutdown();
        if (window_)
        {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
        glfwTerminate();
    }

    int Application::run()
    {
        std::fprintf(stderr, "[run] initialise...\n");
        std::fflush(stderr);
        if (!initialise())
            return 1;

        std::fprintf(stderr, "[run] worldB1.initialise...\n");
        std::fflush(stderr);
        worldB1_.initialise();
        std::fprintf(stderr, "[run] worldB2.initialise...\n");
        std::fflush(stderr);
        worldB2_.initialise();
        worldB1_.spawnVehicle();
        worldB2_.spawnVehicle();
        std::fprintf(stderr, "[run] entering main loop...\n");
        std::fflush(stderr);

        double previousTime = glfwGetTime();
        while (!glfwWindowShouldClose(window_))
        {
            const double currentTime = glfwGetTime();
            const float dt = static_cast<float>(std::min(
                currentTime - previousTime,
                static_cast<double>(AeonLayout::WINDOW_REFRESH_DT_CAP_SECONDS)));
            previousTime = currentTime;

            if (!paused_)
                updateSimulation(dt);

            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(window_, &width, &height);
            if (width <= 0 || height <= 0)
            {
                glfwPollEvents();
                continue;
            }

            renderFrame(width, height);
            updateWindowTitle();
            syncCursorMode();

            glfwSwapBuffers(window_);
            glfwPollEvents();
        }

        close();
        return 0;
    }

    void Application::updateSimulation(float dt)
    {
        ManualControl control;
        if (activeWorld().manualMode())
        {
            if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
                control.steer += 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
                control.steer -= 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
                control.throttle += 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
                control.throttle -= 1.0f;
        }
        activeWorld().updateManualCar(control, dt);
        worldB1_.update(dt);
        worldB2_.update(dt);
        worldB1_.updateSpawning(dt);
        worldB2_.updateSpawning(dt);
        elevator_.update(dt);

        const CameraContext ctx = buildCameraContext(dt);
        camera_.update(ctx);
    }

    CameraContext Application::buildCameraContext(float dt) const
    {
        CameraContext ctx;
        ctx.dt = dt;
        ctx.floorY = activeWorld().floorY();
        ctx.upperFloorY = activeWorld().upperFloorY();
        ctx.floorIndex = activeWorld().floorIndex();

        if (activeWorld().manualMode())
        {
            ctx.hasVehicle = true;
            ctx.vehiclePosition = activeWorld().playerPosition();
            ctx.vehicleYaw = activeWorld().playerYaw();
        }
        else if (!activeWorld().vehicles().empty())
        {
            ctx.hasVehicle = true;
            ctx.vehiclePosition = activeWorld().vehicles().front().position();
            ctx.vehicleYaw = activeWorld().vehicles().front().yaw();
        }

        // Input di chuyển người đi bộ (First-Person Pedestrian Walk)
        if (!activeWorld().manualMode())
        {
            if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS)
                ctx.moveForward += 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS)
                ctx.moveForward -= 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS)
                ctx.moveRight += 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS)
                ctx.moveRight -= 1.0f;
        }
        else
        {
            if (glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS)
                ctx.moveForward += 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS)
                ctx.moveForward -= 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS)
                ctx.moveRight += 1.0f;
            if (glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS)
                ctx.moveRight -= 1.0f;
        }

        ctx.isSprinting = (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                           glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        return ctx;
    }

    void Application::renderFrame(int width, int height)
    {
        const ParkingWorld &world = activeWorld();
        const glm::mat4 viewProjection = camera_.viewProjection(width, height);

        glViewport(0, 0, width, height);
        if (scene_.exteriorVisible())
        {
            glClearColor(0.40f, 0.68f, 0.88f, 1.0f);
        }
        else
        {
            glClearColor(0.14f, 0.16f, 0.18f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer_.beginFrame(viewProjection, glm::vec3(AeonLayout::FRAME_LIGHT_DIR_X, AeonLayout::FRAME_LIGHT_DIR_Y, AeonLayout::FRAME_LIGHT_DIR_Z), camera_.position());
        scene_.render(world, elevator_, camera_.topView());
        renderer_.endScene(); // flush 3D box batches before orthographic HUD

        glDisable(GL_DEPTH_TEST);
        hud_.render(world, scene_.exteriorVisible(), width, height, elevator_, camera_);
        glEnable(GL_DEPTH_TEST);
    }

    void Application::updateWindowTitle()
    {
        char title[500] = {};
        std::snprintf(title, sizeof(title),
                      "AEON B%d Zone A | %d/%d spaces | Free %d | Reserved %d | Occupied %d | Cars %zu | %s | %s",
                      selectedFloor_, AeonLayout::TOTAL_ZONE_A_CAR_SLOTS, AeonLayout::COMPLEX_CAR_CAPACITY_APPROX,
                      activeWorld().freeSlotCount(), activeWorld().reservedSlotCount(), activeWorld().occupiedSlotCount(),
                      static_cast<std::size_t>(activeWorld().carCount() + (activeWorld().manualMode() ? 1 : 0)),
                      activeWorld().freeSlotCount() == 0 ? "FULL" : "AVAILABLE",
                      paused_ ? "PAUSED" : "RUNNING");
        glfwSetWindowTitle(window_, title);
    }

    void Application::handleKey(int key)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
            break;
        case GLFW_KEY_M:
            activeWorld().toggleManualMode();
            break;
        case GLFW_KEY_T:
            camera_.toggleTopView();
            break;
        case GLFW_KEY_U:
            scene_.toggleCeiling();
            break;
        case GLFW_KEY_C:
            if (scene_.exteriorVisible())
            {
                scene_.setExteriorVisible(false);
                selectedFloor_ = AeonLayout::FLOOR_INDEX_B1;
                camera_.focusBasement(worldB1_.floorY());
                camera_.setMode(CameraMode::FirstPersonWalk, buildCameraContext(0.0f));
            }
            else
            {
                camera_.cycleBasementMode(buildCameraContext(0.0f));
            }
            break;
        case GLFW_KEY_K:
        case GLFW_KEY_3:
            if (scene_.exteriorVisible())
            {
                scene_.setExteriorVisible(false);
                selectedFloor_ = AeonLayout::FLOOR_INDEX_B1;
                camera_.focusBasement(worldB1_.floorY());
            }
            camera_.cycleCctvChannel();
            break;
        case GLFW_KEY_4:
            scene_.setExteriorVisible(false);
            camera_.setMode(CameraMode::FirstPersonWalk, buildCameraContext(0.0f));
            break;
        case GLFW_KEY_5:
            scene_.setExteriorVisible(false);
            camera_.setMode(CameraMode::VehicleCockpit, buildCameraContext(0.0f));
            break;
        case GLFW_KEY_6:
            scene_.setExteriorVisible(false);
            camera_.setMode(CameraMode::VehicleChase, buildCameraContext(0.0f));
            break;
        case GLFW_KEY_7:
            scene_.setExteriorVisible(false);
            camera_.setMode(CameraMode::BasementOrbit, buildCameraContext(0.0f));
            break;
        case GLFW_KEY_V:
            if (scene_.exteriorVisible())
            {
                selectedFloor_ = AeonLayout::FLOOR_INDEX_B1;
                scene_.setExteriorVisible(false);
                camera_.focusBasement(worldB1_.floorY());
            }
            else
            {
                selectedFloor_ = AeonLayout::FLOOR_INDEX_B1;
                scene_.setExteriorVisible(true);
                camera_.focusExterior();
            }
            break;
        case GLFW_KEY_1:
            selectedFloor_ = AeonLayout::FLOOR_INDEX_B1;
            scene_.setExteriorVisible(false);
            camera_.focusBasement(worldB1_.floorY());
            break;
        case GLFW_KEY_2:
            selectedFloor_ = AeonLayout::FLOOR_INDEX_B2;
            scene_.setExteriorVisible(false);
            camera_.focusBasement(worldB2_.floorY());
            break;
        case GLFW_KEY_P:
            paused_ = !paused_;
            break;
        case GLFW_KEY_N:
        case GLFW_KEY_SPACE:
            activeWorld().spawnNow();
            break;
        case GLFW_KEY_F:
            activeWorld().toggleAutoSpawn();
            break;
        case GLFW_KEY_B:
            activeWorld().spawnMotorbike();
            break;
        case GLFW_KEY_X:
            activeWorld().removeLastMotorbike();
            break;
        case GLFW_KEY_0:
        case GLFW_KEY_G:
            elevator_.callToFloor(AeonLayout::ELEVATOR_FLOOR_GROUND);
            break;
        case GLFW_KEY_H:
            elevator_.callToFloor(AeonLayout::ELEVATOR_FLOOR_B1);
            break;
        case GLFW_KEY_J:
            elevator_.callToFloor(AeonLayout::ELEVATOR_FLOOR_B2);
            break;
        default:
            break;
        }
    }

    void Application::handleMouseButton(int button, int action)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            dragging_ = (action == GLFW_PRESS);
            glfwGetCursorPos(window_, &lastMouseX_, &lastMouseY_);
        }
    }

    void Application::handleCursor(double x, double y)
    {
        const bool freeLook = camera_.mode() == CameraMode::FirstPersonWalk;

        if (skipNextCursorDelta_)
        {
            // First event after the cursor mode changed: its delta is meaningless.
            skipNextCursorDelta_ = false;
        }
        else if (freeLook)
        {
            // Pedestrian view: the view turns with the mouse, no button needed.
            // Mouse right -> turn right; mouse up -> look up.
            camera_.orbit(static_cast<float>(x - lastMouseX_),
                          -static_cast<float>(y - lastMouseY_));
        }
        else if (dragging_)
        {
            camera_.orbit(static_cast<float>(x - lastMouseX_),
                          static_cast<float>(y - lastMouseY_));
        }
        lastMouseX_ = x;
        lastMouseY_ = y;
    }

    // Hides and captures the cursor while in the pedestrian view so the mouse can
    // keep turning indefinitely; restores it in every other view.
    void Application::syncCursorMode()
    {
        const bool wantCapture =
            camera_.mode() == CameraMode::FirstPersonWalk &&
            glfwGetWindowAttrib(window_, GLFW_FOCUSED) == GLFW_TRUE;
        if (wantCapture == cursorCaptured_)
            return;

        cursorCaptured_ = wantCapture;
        glfwSetInputMode(window_, GLFW_CURSOR,
                         wantCapture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        skipNextCursorDelta_ = true;
        dragging_ = false;
    }

    void Application::handleScroll(double y)
    {
        camera_.zoom(y);
    }

    void Application::onKey(GLFWwindow *window, int key, int, int action, int)
    {
        if (action != GLFW_PRESS)
            return;
        if (auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window)))
        {
            app->handleKey(key);
        }
    }

    void Application::onMouseButton(GLFWwindow *window, int button, int action, int)
    {
        if (auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window)))
        {
            app->handleMouseButton(button, action);
        }
    }

    void Application::onCursorPos(GLFWwindow *window, double x, double y)
    {
        if (auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window)))
        {
            app->handleCursor(x, y);
        }
    }

    void Application::onScroll(GLFWwindow *window, double, double yOffset)
    {
        if (auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window)))
        {
            app->handleScroll(yOffset);
        }
    }

} // namespace

int main()
{
    std::fprintf(stderr, "[boot] Application constructing...\n");
    std::fflush(stderr);
    Application application;
    std::fprintf(stderr, "[boot] Application constructed, calling run()...\n");
    std::fflush(stderr);
    const int rc = application.run();
    std::fprintf(stderr, "[boot] run() returned %d\n", rc);
    std::fflush(stderr);
    return rc;
}
