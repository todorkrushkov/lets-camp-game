#pragma once
// Shared sizing keeps the sprite, collision capsule and eye height consistent.
namespace PlayerDimensions {
constexpr float Scale = 0.85f;
constexpr float Width = 1.5f * Scale;
constexpr float Height = 1.5f * Scale;
constexpr float Radius = 0.22f * Scale;
constexpr float CenterAboveFeet = 0.5f;
constexpr float EyeAboveFeet = Height * 0.9f;
} // namespace PlayerDimensions
