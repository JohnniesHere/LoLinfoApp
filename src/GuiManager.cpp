﻿#include "GUIManager.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <curl/curl.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>
#include <misc/freetype/imgui_freetype.h>


static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

GUIManager::GUIManager()
    : window(nullptr),
    backgroundTexture(0),
    championSplashTexture(0),
    windowOffset(10.0f),
    currentState(WindowState::Default),
    selectedChampionIndex(-1),
    isChampionSplashLoaded(false),
    iconTexture(0),
    isIconLoaded(false),
    skillTextures(5, 0),
    areSkillIconsLoaded(false),
    isRandomizing(false),
    hasRandomChampion(false),
    comboSelectedIndex(-1) {}

GUIManager::~GUIManager() {
    isRandomizing.store(false);
    if (randomizationThread.joinable()) {
        randomizationThread.join();
    }
    Cleanup();
}

bool GUIManager::Initialize(int width, int height, const char* title) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL)
        return false;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    windowSize = ImVec2(width, height);

    // Set the resize callback
    glfwSetWindowSizeCallback(window, WindowResizeCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ApplyCustomStyles();

    // Load and set the custom font
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF("D:\\CPP Project\\CPPProject\\assets\\recharge bd.ttf", 14.0f);
    if (font == nullptr) {
        std::cerr << "Failed to load custom font" << std::endl;
        return false;
    }
    io.FontDefault = font;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
 

    backgroundTexture = LoadTexture("D:\\CPP Project\\CPPProject\\assets\\image.png");

    if (!dataManager.FetchChampionData()) {
        std::cerr << "Failed to fetch champion data" << std::endl;
        return false;
    }

    if (!dataManager.FetchItemData()) {
        std::cerr << "Failed to fetch item data" << std::endl;
        return false;
    }

    if (!LoadIconTexture("D:\\CPP Project\\CPPProject\\assets\\icon.png")) {
        std::cerr << "Failed to load icon texture" << std::endl;
        // Decide if you want to return false here or continue without the icon
    }

    return true;
}

void GUIManager::Render() {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Handle window dragging
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        isDragging = true;
        dragStartPos = ImGui::GetMousePos();
    }
    else if (ImGui::IsMouseReleased(0))
    {
        isDragging = false;
    }

    if (isDragging)
    {
        ImVec2 delta = ImVec2(ImGui::GetMousePos().x - dragStartPos.x, ImGui::GetMousePos().y - dragStartPos.y);

        int x, y;
        glfwGetWindowPos(window, &x, &y);
        glfwSetWindowPos(window, x + delta.x, y + delta.y);
        dragStartPos = ImGui::GetMousePos();
    }

    RenderBackground();
    RenderGUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

bool GUIManager::ShouldClose() {
    return glfwWindowShouldClose(window);
}

void GUIManager::Cleanup() {
    glDeleteTextures(1, &backgroundTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    for (auto& texture : skillTextures) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GUIManager::ApplyCustomStyles() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.FramePadding = ImVec2(10, 10);
    style.ItemSpacing = ImVec2(10, 10);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Button] = ImVec4(0.3608f, 0.3608f, 0.3608f, 0.7f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.1412f, 0.1412f, 0.1412f, 0.9f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
}

GLuint GUIManager::LoadTexture(const char* filename) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return texture;
}

void GUIManager::RenderBackground() {
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void GUIManager::RenderGUI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImColor separatorColor(ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Adjust color as needed

    ImGui::Begin("Full Window", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollWithMouse);

    float windowWidth = ImGui::GetWindowWidth();
    float buttonHeight = 30.0f; // Adjust as needed
    float logoWidth = 32.0f; // Width of your logo
    float closeButtonsWidth = 60.0f; // Width of close and minimize buttons
    float mainButtonsWidth = windowWidth - logoWidth - closeButtonsWidth;
    float sectionWidth = mainButtonsWidth / 3.0f;
    float separatorThickness = 0.5f; // Adjust this value to make the line thicker or thinner

    // Logo
    ImGui::SetCursorPos(ImVec2(0, 0));
    if (isIconLoaded) {
        ImGui::Image((void*)(intptr_t)iconTexture, ImVec2(logoWidth, buttonHeight));
    }


    // Define colors for normal and hovered states
    ImVec4 normalTextColor(1.0f, 1.0f, 1.0f, 1.0f); // White
    ImVec4 hoveredTextColor(0.8431f, 0.7255f, 0.4745f, 1.0f); // Gold/Yellow

    // Push custom styles for main buttons
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0706f, 0.0706f, 0.0706f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1412f, 0.1412f, 0.1412f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

    // Champions button
    ImGui::SetCursorPos(ImVec2(logoWidth + separatorThickness, 0));
    bool championsHovered = ImGui::IsMouseHoveringRect(
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + sectionWidth - separatorThickness, ImGui::GetCursorScreenPos().y + buttonHeight)
    );
    ImGui::PushStyleColor(ImGuiCol_Text, championsHovered ? hoveredTextColor : normalTextColor);
    if (ImGui::Button("CHAMPIONS", ImVec2(sectionWidth - separatorThickness, buttonHeight))) {
        currentState = WindowState::Champions;
    }
    ImGui::PopStyleColor();

    // Items button
    ImGui::SetCursorPos(ImVec2(logoWidth + sectionWidth + separatorThickness, 0));
    bool itemsHovered = ImGui::IsMouseHoveringRect(
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + sectionWidth - separatorThickness, ImGui::GetCursorScreenPos().y + buttonHeight)
    );
    ImGui::PushStyleColor(ImGuiCol_Text, itemsHovered ? hoveredTextColor : normalTextColor);
    if (ImGui::Button("ITEMS", ImVec2(sectionWidth - separatorThickness, buttonHeight))) {
        currentState = WindowState::Items;
    }
    ImGui::PopStyleColor();

    // Summoner's Spells button
    ImGui::SetCursorPos(ImVec2(logoWidth + sectionWidth * 2 + separatorThickness, 0));
    bool spellsHovered = ImGui::IsMouseHoveringRect(
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + sectionWidth - separatorThickness, ImGui::GetCursorScreenPos().y + buttonHeight)
    );
    ImGui::PushStyleColor(ImGuiCol_Text, spellsHovered ? hoveredTextColor : normalTextColor);
    if (ImGui::Button("SUMMONER'S SPELLS", ImVec2(sectionWidth - separatorThickness, buttonHeight))) {
        currentState = WindowState::SummonerSpells;
    }
    ImGui::PopStyleColor();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Draw separator before close buttons
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(windowWidth - closeButtonsWidth - separatorThickness, 0),
        ImVec2(windowWidth - closeButtonsWidth, buttonHeight),
        separatorColor
    );

    // Close and minimize buttons
    ImGui::SetCursorPos(ImVec2(windowWidth - closeButtonsWidth, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

    // Horizontal separator below all buttons
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(0, buttonHeight),
        ImVec2(windowWidth, buttonHeight),
        separatorColor
    );


    if (ImGui::Button("--", ImVec2(30, buttonHeight))) {
        glfwIconifyWindow(window);
    }
    ImGui::SameLine(0, 0);
    if (ImGui::Button("X", ImVec2(30, buttonHeight))) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Handle dragging (allow dragging from the top bar)
    if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(ImVec2(0, 0), ImVec2(windowWidth, buttonHeight))) {
        isDragging = true;
        dragStartPos = ImGui::GetMousePos();
    }
    else if (ImGui::IsMouseReleased(0)) {
        isDragging = false;
    }

    if (isDragging) {
        ImVec2 delta = ImVec2(ImGui::GetMousePos().x - dragStartPos.x, ImGui::GetMousePos().y - dragStartPos.y);
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        glfwSetWindowPos(window, x + delta.x, y + delta.y);
        dragStartPos = ImGui::GetMousePos();
    }

    // Add a small resize handle in the bottom-right corner
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 resizeHandlePos = ImVec2(windowPos.x + windowSize.x - 20, windowPos.y + windowSize.y - 20);
    ImGui::GetWindowDrawList()->AddTriangleFilled(
        ImVec2(resizeHandlePos.x + 20, resizeHandlePos.y + 20),
        ImVec2(resizeHandlePos.x + 20, resizeHandlePos.y),
        ImVec2(resizeHandlePos.x, resizeHandlePos.y + 20),
        IM_COL32(200, 200, 200, 255));

    // Handle resizing
    if (ImGui::IsMouseHoveringRect(resizeHandlePos, ImVec2(resizeHandlePos.x + 20, resizeHandlePos.y + 20))) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsMouseClicked(0)) {
            isResizing = true;
            resizeStartPos = ImGui::GetMousePos();
        }
    }

    if (ImGui::IsMouseReleased(0)) {
        isResizing = false;
    }

    if (isResizing) {
        ImVec2 delta = ImVec2(ImGui::GetMousePos().x - resizeStartPos.x, ImGui::GetMousePos().y - resizeStartPos.y);
        windowSize.x += delta.x;
        windowSize.y += delta.y;
        glfwSetWindowSize(window, windowSize.x, windowSize.y);
        resizeStartPos = ImGui::GetMousePos();
    }

    // Render the appropriate window based on the current state
    ImGui::SetCursorPos(ImVec2(0, buttonHeight));
    ImGui::BeginChild("Content Window", ImVec2(viewport->Size.x, viewport->Size.y - buttonHeight), false,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

    switch (currentState) {
    case WindowState::Champions:
        RenderChampionsWindow();
        break;
    case WindowState::Items:
        RenderItemsWindow();
        break;
    case WindowState::SummonerSpells:
        ImGui::Text("This feature is not implemented yet.");
        break;
    default:
        RenderDefaultWindow();
        break;
    }

    ImGui::PopStyleColor();
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void GUIManager::RenderDefaultWindow() {
    ImVec2 textSize = ImGui::CalcTextSize("Select one option above");
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImVec2 cursorPos(
        (windowSize.x - textSize.x) * 0.5f,
        (windowSize.y - textSize.y) * 0.5f
    );

    ImGui::SetCursorPos(cursorPos);
    ImGui::Text("Select one option above");
}

void GUIManager::RenderChampionsWindow() {
    const auto& championNames = dataManager.GetChampionNames();

    // Display champion splash art as background if a champion is selected
    if (isChampionSplashLoaded && selectedChampionIndex >= 0) {
        ImGui::GetWindowDrawList()->AddImage(
            (void*)(intptr_t)championSplashTexture,
            ImGui::GetWindowPos(),
            ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 128)  // 50% opacity
        );
    }

    // Create a semi-transparent overlay for the controls
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
    ImGui::BeginChild("ControlsOverlay", ImVec2(ImGui::GetWindowWidth(), 60), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);


    // Set custom colors
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));         // Dark background for input fields
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));  // Slightly lighter when hovered
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));   // Even lighter when active
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));         // Dark background for combo popup
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));            // White text
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));          // Slightly lighter background for selected item
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));   // Even lighter for hovered item
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));    // Lightest for active/clicked item
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));          // Dark button color
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));   // Lighter button color when hovered
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));    // Even lighter when clicked



    // Champion selection dropdown with search
    static char searchBuffer[256] = "";
    ImGui::SetNextItemWidth(300); // Set the width of the combo box to 300 pixels
    ImGui::SetCursorPos(ImVec2(10, 5));
    if (ImGui::BeginCombo("##ChampionSelect", selectedChampionIndex >= 0 ? championNames[selectedChampionIndex].c_str() : "Select Champion")) {
        // Add a search input field at the top of the combo box
        ImGui::PushItemWidth(-1);
        ImGui::SetCursorPos(ImVec2(10, 10));
        if (ImGui::InputText("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer))) {
            // Convert search to lowercase for case-insensitive comparison
            std::string search = searchBuffer;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        }
        ImGui::PopItemWidth();
        ImGui::Separator();

        for (int i = 0; i < championNames.size(); i++) {
            // Convert champion name to lowercase for case-insensitive comparison
            std::string lowerChampName = championNames[i];
            std::transform(lowerChampName.begin(), lowerChampName.end(), lowerChampName.begin(), ::tolower);

            // Only display champions that match the search
            if (lowerChampName.find(searchBuffer) != std::string::npos) {
                bool is_selected = (selectedChampionIndex == i);
                if (ImGui::Selectable(championNames[i].c_str(), is_selected)) {
                    if (selectedChampionIndex != i) {  // Check if a different champion is selected
                        selectedChampionIndex = i;
                        std::string championId = dataManager.GetChampionId(championNames[i]);
                        LoadChampionSplash(championId);
                        LoadChampionIcon(championId);
                        areSkillIconsLoaded = false;
                        selectedSkill = ""; // Reset selected skill when changing champion
                        skillDescription = ""; // Clear skill description
                        // Reset tip-related states
                        showAllyTip = false;
                        showEnemyTip = false;
                        allyTips.clear();
                        enemyTips.clear();
                        allyTipIndices.clear();
                        enemyTipIndices.clear();
                        currentAllyTipIndex = 0;
                        currentEnemyTipIndex = 0;
                    }
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Pop custom colors
    ImGui::PopStyleColor(11);

    // Add Random Champion button
    ImGui::SameLine();
    if (ImGui::Button("Random Champion")) {
        if (!isRandomizing.load()) {
            RandomizeChampion();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // Pop the ChildBg color

    // Load the randomly selected champion
    if (!isRandomizing.load() && hasRandomChampion.load()) {
        hasRandomChampion.store(false); // Reset the flag

        std::string championName = championNames[selectedChampionIndex];
        std::string championId = dataManager.GetChampionId(championName);
        LoadChampionSplash(championId);
        LoadChampionIcon(championId);
        areSkillIconsLoaded = false;
        selectedSkill = "";
        skillDescription = "";
        showAllyTip = false;
        showEnemyTip = false;
        allyTips.clear();
        enemyTips.clear();
        allyTipIndices.clear();
        enemyTipIndices.clear();
        currentAllyTipIndex = 0;
        currentEnemyTipIndex = 0;
    }

    // Display champion splash art as background
    if (isChampionSplashLoaded && selectedChampionIndex >= 0) {
        ImGui::GetWindowDrawList()->AddImage(
            (void*)(intptr_t)championSplashTexture,
            ImGui::GetWindowPos(),
            ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 128)  // 50% opacity
        );
    }

    if (selectedChampionIndex >= 0) {
        std::string championName = championNames[selectedChampionIndex];
        std::string championId = dataManager.GetChampionId(championName);

        // Display champion icon
        if (isChampionIconLoaded) {
            ImGui::SetCursorPos(ImVec2(10, 50));
            ImGui::Image((void*)(intptr_t)championIconTexture, ImVec2(64, 64));
        }

        // Display champion info
        ImGui::SetCursorPos(ImVec2(10, 120));
        ImGui::BeginChild("ChampionInfo", ImVec2(300, 500), true, ImGuiWindowFlags_NoScrollbar);

        nlohmann::json stats = dataManager.GetChampionStats(championName);
        std::string title = dataManager.GetChampionTitle(championName);
        std::string lore = dataManager.GetChampionLore(championName);
        auto tags = dataManager.GetChampionTags(championName);

        ImGui::Text("Champion: %s", championName.c_str());
        ImGui::Text("Title: %s", title.c_str());
        ImGui::Text("Tags: ");
        for (const auto& tag : tags) {
            ImGui::SameLine();
            ImGui::Text("%s", tag.c_str());
        }
        ImGui::Text("Base Stats:");
        ImGui::Text("HP: %.0f (+ %.0f per level)", stats["hp"].get<float>(), stats["hpperlevel"].get<float>());
        ImGui::Text("Armor: %.1f (+ %.2f per level)", stats["armor"].get<float>(), stats["armorperlevel"].get<float>());
        ImGui::Text("Magic Resist: %.1f (+ %.2f per level)", stats["spellblock"].get<float>(), stats["spellblockperlevel"].get<float>());
        ImGui::Text("Move Speed: %.0f", stats["movespeed"].get<float>());
        ImGui::Text("Attack Damage: %.0f (+ %.0f per level)", stats["attackdamage"].get<float>(), stats["attackdamageperlevel"].get<float>());
        ImGui::Text("Attack Speed: %.3f (+ %.1f%% per level)", stats["attackspeed"].get<float>(), stats["attackspeedperlevel"].get<float>());
        ImGui::Text("Attack Range: %.0f", stats["attackrange"].get<float>());
        ImGui::Text("HP Regen: %.1f (+ %.1f per level)", stats["hpregen"].get<float>(), stats["hpregenperlevel"].get<float>());

        ImGui::EndChild();

        // Display champion lore
        ImGui::SetCursorPos(ImVec2(320, 120));  // Adjust position as needed
        ImGui::BeginChild("ChampionLore", ImVec2(ImGui::GetWindowWidth() - 330, 100), true, ImGuiWindowFlags_HorizontalScrollbar);


        ImGui::TextWrapped("%s", lore.c_str());

        ImGui::EndChild();

        // Load and display skill icons
        if (!areSkillIconsLoaded) {
            LoadSkillIcons(championId);
        }

        // Display skill icons and buttons
        ImGui::SetCursorPos(ImVec2(320, 230));
        std::string skillNames[] = { "Passive", "Q", "W", "E", "R" };
        for (int i = 0; i < 5; ++i) {
            ImGui::BeginGroup();
            if (skillTextures[i] != 0) {
                ImGui::Image((void*)(intptr_t)skillTextures[i], ImVec2(75, 75));
            }
            std::string buttonLabel = (i == 0) ? "Passive" : skillNames[i] + " Ability";

            // Check if this skill is currently selected
            bool isSelected = (selectedSkill == (i == 0 ? "Passive" : championId + " " + skillNames[i]));

            // Use PushStyleColor to change button color if selected
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8431f, 0.7255f, 0.4745f, 1.0f)); // Green color for selected skill
            }

            if (ImGui::Button(buttonLabel.c_str(), ImVec2(75, 35))) {
                if (isSelected) {
                    // If already selected, deselect it
                    selectedSkill = "";
                    skillDescription = "";
                }
                else {
                    // Select this skill
                    selectedSkill = (i == 0) ? "Passive" : championId + " " + skillNames[i];
                    skillDescription = skillDescriptions[selectedSkill];
                }
            }

            // Pop the style color if we pushed it
            if (isSelected) {
                ImGui::PopStyleColor();
            }

            ImGui::EndGroup();
            if (i < 4) ImGui::SameLine(0, 20);
        }

        // Display skill description
        if (!selectedSkill.empty()) {
            ImGui::SetCursorPos(ImVec2(320, 360)); // Adjusted position
            ImGui::BeginChild("SkillDescription", ImVec2(ImGui::GetWindowWidth() - 330, 70), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextWrapped("%s", skillDescription.c_str());
            ImGui::EndChild();
        }

        // Add Skins button
        ImGui::SetCursorPos(ImVec2(320, 440)); // Adjust this position as needed
        if (ImGui::Button("Skins")) {
            showSkins = !showSkins; // Toggle skins display
            if (showSkins) {
                currentSkinIndex = 0; // Reset to first skin when showing skins
            }
        }

        // Display skins if showSkins is true
        if (showSkins) {
            auto skins = dataManager.GetChampionSkins(championName);
            if (!skins.empty()) {
                const auto& currentSkin = skins[currentSkinIndex];
                std::string skinName = currentSkin["name"];
                std::string skinNum = std::to_string(currentSkin["num"].get<int>());
                std::string skinKey = championId + "_" + skinNum;

                // Load skin texture if not already loaded
                if (skinTextures.find(skinKey) == skinTextures.end()) {
                    std::string skinImageUrl = dataManager.GetChampionSkinImageUrl(championId, skinNum);
                    skinTextures[skinKey] = LoadSkinTexture(skinImageUrl);
                }

                // Display skin image
                ImGui::SetCursorPos(ImVec2(390, 440)); // Adjust position as needed
                ImGui::Image((void*)(intptr_t)skinTextures[skinKey], ImVec2(240, 136)); // Adjust size as needed

                // Display skin name in a chat box style
                ImGui::SetCursorPos(ImVec2(390, 580)); // Adjusted position
                ImGui::BeginChild("SkinName", ImVec2(240, 40), true);
                ImGui::Text("%s", skinName.c_str());
                ImGui::EndChild();

                // Navigation buttons outside the chat box
                ImGui::SetCursorPos(ImVec2(390, 625)); // Adjusted position
                if (currentSkinIndex > 0) {
                    if (ImGui::ArrowButton("##left", ImGuiDir_Left)) {
                        currentSkinIndex--;
                    }
                    ImGui::SameLine();
                }

                ImGui::SetCursorPos(ImVec2(595, 625)); // Adjusted position for right arrow
                if (currentSkinIndex < skins.size() - 1) {
                    if (ImGui::ArrowButton("##right", ImGuiDir_Right)) {
                        currentSkinIndex++;
                    }
                }
            }
        }

        // Add Ally Tip button
        ImGui::SetCursorPos(ImVec2(650, 440));
        if (ImGui::Button("Ally Tips")) {
            showAllyTip = !showAllyTip;
            if (showAllyTip && allyTips.empty()) {
                allyTips = dataManager.GetChampionAllyTips(championName);
                if (!allyTips.empty()) {
                    RandomizeTips(allyTips, allyTipIndices);
                    currentAllyTipIndex = 0;
                }
            }
        }

        if (showAllyTip) {
            ImGui::SetCursorPos(ImVec2(650, 480));
            if (!allyTips.empty()) {
                if (ImGui::Button("Next Ally Tip")) {
                    currentAllyTipIndex = (currentAllyTipIndex + 1) % allyTipIndices.size();
                }
            }

            ImGui::SetCursorPos(ImVec2(795, 440));
            ImGui::BeginChild("AllyTip", ImVec2(240, 90), true);
            if (!allyTips.empty()) {
                size_t index = allyTipIndices[currentAllyTipIndex];
                ImGui::TextWrapped("%s", allyTips[index].c_str());
            }
            else {
                ImGui::TextWrapped("No ally tips available for this champion.");
            }
            ImGui::EndChild();
        }

        // For Enemy Tips
        ImGui::SetCursorPos(ImVec2(650, 535));
        if (ImGui::Button("Enemy Tips")) {
            showEnemyTip = !showEnemyTip;
            if (showEnemyTip && enemyTips.empty()) {
                enemyTips = dataManager.GetChampionEnemyTips(championName);
                if (!enemyTips.empty()) {
                    RandomizeTips(enemyTips, enemyTipIndices);
                    currentEnemyTipIndex = 0;
                }
            }
        }

        if (showEnemyTip) {
            ImGui::SetCursorPos(ImVec2(650, 575));
            if (!enemyTips.empty()) {
                if (ImGui::Button("Next Enemy Tip")) {
                    currentEnemyTipIndex = (currentEnemyTipIndex + 1) % enemyTipIndices.size();
                }
            }

            ImGui::SetCursorPos(ImVec2(795, 575));
            ImGui::BeginChild("EnemyTip", ImVec2(240, 80), true);
            if (!enemyTips.empty()) {
                size_t index = enemyTipIndices[currentEnemyTipIndex];
                ImGui::TextWrapped("%s", enemyTips[index].c_str());
            }
            else {
                ImGui::TextWrapped("No enemy tips available for this champion.");
            }
            ImGui::EndChild();
        }
    }
}


void GUIManager::CleanupSkinTextures() {
    for (auto& pair : skinTextures) {
        glDeleteTextures(1, &pair.second);
    }
    skinTextures.clear();
}

void GUIManager::LoadChampionSplash(const std::string& championName) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string url = dataManager.GetChampionImageUrl(championName);
        std::string imageData;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            int width, height, channels;
            unsigned char* image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image) {
                if (isChampionSplashLoaded) {
                    glDeleteTextures(1, &championSplashTexture);
                }

                glGenTextures(1, &championSplashTexture);
                glBindTexture(GL_TEXTURE_2D, championSplashTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

                stbi_image_free(image);
                isChampionSplashLoaded = true;
            }
        }
    }
}

void GUIManager::LoadChampionIcon(const std::string& championName) {
    // Similar to LoadChampionSplash, but for the icon
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string url = dataManager.GetChampionIconUrl(championName);  // You'll need to add this function to DataManager
        std::string imageData;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            int width, height, channels;
            unsigned char* image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image) {
                if (isChampionIconLoaded) {
                    glDeleteTextures(1, &championIconTexture);
                }

                glGenTextures(1, &championIconTexture);
                glBindTexture(GL_TEXTURE_2D, championIconTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

                stbi_image_free(image);
                isChampionIconLoaded = true;
            }
        }
    }
}

size_t GUIManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}



void GUIManager::WindowResizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

bool GUIManager::LoadIconTexture(const char* filename) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 4);
    if (image == nullptr) {
        std::cerr << "Failed to load icon image: " << filename << std::endl;
        return false;
    }

    glGenTextures(1, &iconTexture);
    glBindTexture(GL_TEXTURE_2D, iconTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    stbi_image_free(image);
    isIconLoaded = true;
    return true;
}

void GUIManager::LoadSkillIcons(const std::string& championId) {
    auto spells = dataManager.GetChampionSpells(championId);
    auto passive = dataManager.GetChampionPassive(championId);

    // Load passive icon and description
    LoadSkillIcon(passive["image"]["full"], 0);
    skillDescriptions["Passive"] = passive["name"].get<std::string>() + ": " + passive["description"].get<std::string>();

    // Load skill icons and descriptions
    std::string skillNames[] = { "Q", "W", "E", "R" };
    for (int i = 0; i < spells.size() && i < 4; ++i) {
        LoadSkillIcon(spells[i]["image"]["full"], i + 1);
        skillDescriptions[championId + " " + skillNames[i]] =
            spells[i]["name"].get<std::string>() + ": " + spells[i]["description"].get<std::string>();
    }

    areSkillIconsLoaded = true;
}

void GUIManager::LoadSkillIcon(const std::string& iconFilename, int index) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string url = "http://ddragon.leagueoflegends.com/cdn/14.14.1/img/passive/" + iconFilename;
        if (index > 0) {
            url = "http://ddragon.leagueoflegends.com/cdn/14.14.1/img/spell/" + iconFilename;
        }
        std::string imageData;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            int width, height, channels;
            unsigned char* image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image) {
                if (skillTextures[index] != 0) {
                    glDeleteTextures(1, &skillTextures[index]);
                }

                glGenTextures(1, &skillTextures[index]);
                glBindTexture(GL_TEXTURE_2D, skillTextures[index]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

                stbi_image_free(image);
            }
        }
    }
}

GLuint GUIManager::LoadSkinTexture(const std::string& url) {
    CURL* curl = curl_easy_init();
    GLuint texture = 0;
    if (curl) {
        std::string imageData;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            int width, height, channels;
            unsigned char* image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image) {
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                stbi_image_free(image);
            }
            else {
                std::cerr << "Failed to load skin image: " << url << std::endl;
            }
        }
        else {
            std::cerr << "Failed to download skin image: " << url << std::endl;
        }
    }
    return texture;
}

void GUIManager::RandomizeTips(const std::vector<std::string>& tips, std::vector<size_t>& indices) {
    std::lock_guard<std::mutex> lock(tipMutex);

    indices.resize(tips.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(indices.begin(), indices.end(), g);
}

void GUIManager::RandomizeChampion() {
    if (isRandomizing.load()) return; // Don't start a new randomization if one is in progress

    isRandomizing.store(true);
    hasRandomChampion.store(false);
    const auto& championNames = dataManager.GetChampionNames();

    randomizationThread = std::thread([this, championNames]() {
        std::vector<size_t> indices(championNames.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(indices.begin(), indices.end(), g);

        // Select a single random champion
        {
            std::lock_guard<std::mutex> lock(tipMutex);
            selectedChampionIndex = indices[0];
        }

        isRandomizing.store(false);
        hasRandomChampion.store(true);
        });

    randomizationThread.detach(); // Allow the thread to run independently
}

// Item window functions implementation

void GUIManager::RenderItemsWindow() {
    const auto& itemNames = dataManager.GetItemNames();
    if (itemNames.empty()) {
        ImGui::Text("No items available.");
        return;
    }

    static char searchBuffer[256] = "";
    ImGui::SetNextItemWidth(300);
    ImGui::SetCursorPos(ImVec2(10, 5));

    if (ImGui::BeginCombo("##ItemSelect",
        (comboSelectedIndex >= 0 && comboSelectedIndex < itemNames.size())
        ? itemNames[comboSelectedIndex].c_str()
        : "Select Item")) {

        ImGui::PushItemWidth(-1);
        ImGui::SetCursorPos(ImVec2(10, 10));
        if (ImGui::InputText("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer))) {
            std::string search = searchBuffer;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        }
        ImGui::PopItemWidth();
        ImGui::Separator();

        for (int i = 0; i < itemNames.size(); i++) {
            std::string lowerItemName = itemNames[i];
            std::transform(lowerItemName.begin(), lowerItemName.end(), lowerItemName.begin(), ::tolower);

            if (lowerItemName.find(searchBuffer) != std::string::npos) {
                bool is_selected = (comboSelectedIndex == i);
                if (ImGui::Selectable(itemNames[i].c_str(), is_selected)) {
                    if (comboSelectedIndex != i) {
                        comboSelectedIndex = i;
                        std::string itemId = dataManager.GetItemId(itemNames[i]);
                        std::cout << "Selected item from combo: " << itemNames[i] << " (ID: " << itemId << ")" << std::endl;
                        // Update currentItems and selectedItemIndex for consistency
                        currentItems = { itemId };
                        selectedItemIndex = 0;
                    }
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Tag Buttons
    ImGui::NewLine();

    // Back button
    ImGui::SetCursorPos(ImVec2(10, ImGui::GetCursorPosY()));
    if (ImGui::Button("Back")) {
        if (!itemHistory.empty()) {
            ItemHistoryEntry previous = itemHistory.back();
            itemHistory.pop_back();

            currentItems = { previous.itemId };
            selectedItemIndex = 0;
            comboSelectedIndex = -1;
            currentItemTags = previous.tags;

            // Use the original tag instead of the last tag in the list
            currentTag = previous.originalTag;

            if (!currentTag.empty()) {
                // If we're going back to a tag view, reload items for that tag
                DisplayItemsByTag(currentTag);
            }
        }
        else if (!currentTag.empty()) {
            // If history is empty but we have a tag, go back to tag view
            DisplayItemsByTag(currentTag);
        }
    }

    // Calculate the width available for tag buttons
    float windowWidth = ImGui::GetWindowWidth();
    float backButtonWidth = ImGui::GetItemRectSize().x;
    float availableWidth = windowWidth - backButtonWidth - 20; // 20 for some padding

    ImGui::SameLine();

    ImGui::SetCursorPosX(backButtonWidth + 20); // Position after the Back button with some padding
    const char* tags[] = { "FIGHTER", "MARKSMAN", "ASSASSIN", "MAGE", "TANK", "SUPPORT" };
    int numTags = IM_ARRAYSIZE(tags);
    float tagButtonWidth = availableWidth / numTags;

    for (int i = 0; i < numTags; i++) {
        if (i > 0) ImGui::SameLine();
        ImGui::SetCursorPosX(backButtonWidth + 20 + i * tagButtonWidth);

        bool isActiveTag = (currentTag == tags[i]);
        if (isActiveTag) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f)); // Green color for active tag
        }

        if (ImGui::Button(tags[i], ImVec2(tagButtonWidth - 5, 0))) { // -5 for small gap between buttons
            std::cout << tags[i] << " button clicked" << std::endl;
            try {
                DisplayItemsByTag(tags[i]);
            }
            catch (const std::exception& e) {
                std::cerr << "Exception when displaying items for tag " << tags[i] << ": " << e.what() << std::endl;
            }
        }

        if (isActiveTag) {
            ImGui::PopStyleColor();
        }
    }

    // Display items for the selected tag or combo box selection
    // Set the cursor position to move the child window to the right
    ImGui::SetCursorPos(ImVec2(25, ImGui::GetCursorPosY())); // Adjust the 25 to move it more or less to the right

    // Now begin the child window
    ImGui::BeginChild("ItemsList", ImVec2(ImGui::GetWindowWidth() - 50, 300), true); // Adjusted width to account for the moved position    std::cout << "Number of items to display: " << currentItems.size() << std::endl;
    int itemsPerRow = 13;
    for (int i = 0; i < currentItems.size(); i++) {
        const auto& itemId = currentItems[i];
        std::string itemName = dataManager.GetSpecificItemName(itemId);
        std::string itemIconUrl = dataManager.GetItemImageUrl(itemId);

        GLuint itemTexture = LoadTextureFromURL(itemIconUrl);

        if (i % itemsPerRow != 0) ImGui::SameLine();
        if (ImGui::ImageButton((void*)(intptr_t)itemTexture, ImVec2(64, 64))) {
            selectedItemIndex = i;
            std::cout << "Selected item from grid: " << itemName << " (ID: " << itemId << ")" << std::endl;

            // Add this item to history with its current context
            if (!itemHistory.empty() && itemHistory.back().itemId != itemId) {
                itemHistory.push_back({ itemId, currentItemTags, currentTag });
            }
            // Update current item tags
            currentItemTags = dataManager.GetItemTags(itemId);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", itemName.c_str());
            ImGui::EndTooltip();
        }
    }
    ImGui::EndChild();

    // Display item details
    if (!currentItems.empty() && selectedItemIndex >= 0 && selectedItemIndex < currentItems.size()) {
        std::string itemId = currentItems[selectedItemIndex];
        currentItemTags = dataManager.GetItemTags(itemId);
        // Adjust the layout for two columns
        float columnWidth = (ImGui::GetWindowWidth() - 50) / 2;  // Subtracting 50 for padding
        // Capture the cursor position before rendering the stats window
        ImVec2 statsWindowPos = ImGui::GetCursorPos();

        // Item stats column
        ImGui::SetCursorPos(ImVec2(25, ImGui::GetCursorPosY()));
        ImGui::BeginChild("ItemDetails", ImVec2(columnWidth - 10, 200), true);

        ImGui::Text("Name: %s", dataManager.GetSpecificItemName(itemId).c_str());
        ImGui::Text("Description: %s", dataManager.GetItemDescription(itemId).c_str());
        int cost = dataManager.GetItemCost(itemId);
        if (cost >= 0) {
            ImGui::Text("Cost: %d", cost);
        }
        auto stats = dataManager.GetItemStats(itemId);
        if (!stats.empty()) {
            ImGui::Text("Stats:");
            for (auto& [statName, statValue] : stats.items()) {
                if (statValue.contains("flat") && statValue["flat"].get<float>() != 0) {
                    ImGui::Text("  %s: %.2f", statName.c_str(), statValue["flat"].get<float>());
                }
            }
        }

        ImGui::EndChild();
        // Capture the size of the stats window
        ImVec2 statsWindowSize = ImGui::GetItemRectSize();
        float itemDetailsHeight = ImGui::GetItemRectSize().y;  // Get the actual height of the item details section
        
        // Builds Into column
        ImGui::SetCursorPos(ImVec2(25 + columnWidth, statsWindowPos.y));
        ImGui::BeginChild("BuildsInto", ImVec2(columnWidth - 10, statsWindowSize.y), true);
        ImGui::Text("Builds Into:");

        std::vector<std::string> buildsInto = dataManager.GetItemBuildsInto(itemId);
        std::vector<std::string> buildsInto = dataManager.GetItemBuildsInto(itemId);
        if (!buildsInto.empty()) {
            for (const auto& buildItemId : buildsInto) {
                try {
                    std::string buildItemName = dataManager.GetSpecificItemName(buildItemId);
                    std::string buildItemIconUrl = dataManager.GetItemImageUrl(buildItemId);
                    GLuint buildItemTexture = LoadTextureFromURL(buildItemIconUrl);
                    if (ImGui::ImageButton((void*)(intptr_t)buildItemTexture, ImVec2(32, 32))) {
                        // Add current item to history before navigating
                        itemHistory.push_back({ itemId, currentItemTags, currentTag });

                        // Update current item
                        currentItems = { buildItemId };
                        selectedItemIndex = 0;
                        comboSelectedIndex = -1;

                        // Update tags for the new item
                        currentItemTags = dataManager.GetItemTags(buildItemId);

                        // Clear current tag as we're now viewing an individual item
                        currentTag = "";

                        std::cout << "Selected item from 'Builds Into': " << buildItemName << " (ID: " << buildItemId << ")" << std::endl;
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", buildItemName.c_str());
                    ImGui::Separator();
                }
                catch (const std::exception& e) {
                    ImGui::Text("Error loading item %s: %s", buildItemId.c_str(), e.what());
                }
            }
        }

        ImGui::EndChild();

        // Display item tags
        ImGui::Text("Tags:");
        for (const auto& tag : currentItemTags) {
            ImGui::SameLine();
            bool isOriginalTag = (tag == currentTag);
            if (isOriginalTag) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
            }
            if (ImGui::SmallButton(tag.c_str())) {
                DisplayItemsByTag(tag);
            }
            if (isOriginalTag) {
                ImGui::PopStyleColor();
            }
        }
    }
}

void GUIManager::RenderItemsDetail() {
    static std::string selectedTag = "DefaultTag"; // Set a default tag
    static std::vector<std::string> tags = { "Fighter", "Marksman", "Assassin", 
                                             "Mage", "Tank", "Support"};

    // Combo box for selecting item categories/tags
    if (ImGui::BeginCombo("Select Tag", selectedTag.c_str())) {
        for (const auto& tag : tags) {
            bool isSelected = (selectedTag == tag);
            if (ImGui::Selectable(tag.c_str(), isSelected)) {
                selectedTag = tag;
                DisplayItemsByTag(selectedTag); // Load items for the selected tag
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Display item icons as buttons
    for (const auto& itemId : currentItems) {
        std::string itemIconUrl = dataManager.GetItemImageUrl(itemId);
        GLuint itemTexture = LoadTextureFromURL(itemIconUrl); // Load the texture
        if (ImGui::ImageButton((void*)(intptr_t)itemTexture, ImVec2(64, 64))) {
            // Handle item button click, e.g., show item details
            std::cout << "Item clicked: " << itemId << std::endl;
        }
    }
}

void GUIManager::DisplayItemsByTag(const std::string& tag) {
    std::cout << "DisplayItemsByTag called with tag: " << tag << std::endl;
    currentItems = dataManager.GetItemsByTag(tag);
    selectedItemIndex = -1;
    itemHistory.clear();  // Clear history when changing tags
    currentTag = tag;  // Set the current tag
    comboSelectedIndex = -1;  // Reset combo box selection
    currentItemTags.clear();  // Clear current item tags
    std::cout << "Items loaded for tag " << tag << ": " << currentItems.size() << std::endl;
}

// Simplified function to load texture from URL
GLuint GUIManager::LoadTextureFromURL(const std::string& url) {
    // Check if the texture is already loaded
    if (itemTextures.find(url) != itemTextures.end()) {
        return itemTextures[url];
    }

    CURL* curl = curl_easy_init();
    GLuint texture = 0;
    if (curl) {
        std::string imageData;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            int width, height, channels;
            unsigned char* image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image) {
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                stbi_image_free(image);

                // Cache the loaded texture
                itemTextures[url] = texture;
            }
        }
    }
    return texture;
}
