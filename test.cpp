#include <iostream>
#include <thread>
#include <vector>
#include <cmath>

#include "Memory/MemoryManager.h"
#include "globals.h"
#include "Utils/console.h"

// LookAtTarget 함수: 카메라가 대상의 Head를 바라보도록 회전
void LookAtTarget(RobloxInstance& camera, const Vectors::Vector3& targetPos) {
    Vectors::Vector3 camPos = camera.CFrame().Position();

    // 방향 벡터: 카메라에서 대상까지
    Vectors::Vector3 forward = (targetPos - camPos).Normalize();

    // Roblox는 -Z 방향이 forward이므로, 이 벡터는 반전해야 함
    forward.x = -forward.x;

    Vectors::Vector3 up = { 0, 1, 0 };
    Vectors::Vector3 right = up.cross(forward).Normalize();
    up = forward.cross(right).Normalize();

    sCFrame cf = {
        right.x, up.x, forward.x,   // r00 r01 r02
        right.y, up.y, forward.y,   // r10 r11 r12
        right.z, up.z, forward.z,   // r20 r21 r22
        camPos.x, camPos.y, camPos.z  // position
    };

    camera.SetCFrame(cf);
}



// 가장 가까운 플레이어 Head의 위치를 반환
Vectors::Vector3 FindClosestPlayerHeadPosition() {
    RobloxInstance localPlayer = Globals::Roblox::LocalPlayer;
    Vectors::Vector3 myPos = localPlayer.Character().FindFirstChild("HumanoidRootPart").Position();

    float closestDist = FLT_MAX;
    Vectors::Vector3 closestPos;

    for (auto& player : Globals::Roblox::Players.GetChildren()) {
        if (player == localPlayer) continue;

        RobloxInstance head = player.Character().FindFirstChild("Head");
        if (!head) continue;

        Vectors::Vector3 headPos = head.Position();
        float dist = (myPos - headPos).Magnitude();

        if (dist < closestDist) {
            closestDist = dist;
            closestPos = headPos;
        }
    }

    return closestPos;
}

// 모든 카메라를 회전시킴
void AimAllCamerasAtClosestPlayerHead(const std::vector<RobloxInstance>& cameras) {
    //Vectors::Vector3 targetPos = FindClosestPlayerHeadPosition();
    Vectors::Vector3 targetPos = {0,0,0};

    for (auto cam : cameras) {
        LookAtTarget(cam, targetPos);
    }
}

// 프로그램 시작
int main() {
    if (!Memory->attachToProcess("RobloxPlayerBeta.exe")) {
        log("Failed to attach", 2);
        return 1;
    }

    auto fakeDataModel = Memory->read<uintptr_t>(Memory->getBaseAddress() + offsets::FakeDataModelPointer);
    auto dataModelAddr = Memory->read<uintptr_t>(fakeDataModel + offsets::FakeDataModelToDataModel);
    Globals::Roblox::DataModel = RobloxInstance(dataModelAddr);
    Globals::Roblox::Players = Globals::Roblox::DataModel.FindFirstChildWhichIsA("Players");
    Globals::Roblox::LocalPlayer = Globals::Roblox::Players.FindFirstChildWhichIsA("Player");

    RobloxInstance workspace = Globals::Roblox::DataModel.FindFirstChild("Workspace");
    if (!workspace) {
        log("Workspace not found", 2);
        return 1;
    }

    std::vector<RobloxInstance> cameras;
    for (auto& child : workspace.GetChildren()) {
        if (child.Class() == "Camera") {
            cameras.push_back(child);
            std::cout << "Found camera at: 0x" << std::hex << child.address << "\n";
        }
    }

    if (cameras.empty()) {
        log("No cameras found!", 2);
        return 1;
    }

    // 매초마다 카메라 회전
    while (true) {
        AimAllCamerasAtClosestPlayerHead(cameras);
        std::this_thread::sleep_for(std::chrono::seconds(0));
    }

    return 0;
}
