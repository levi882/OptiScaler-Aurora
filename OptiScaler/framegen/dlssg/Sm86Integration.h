#pragma once

namespace Sm86
{
// Explicit opt-in only. Called before Aurora installs its own loader hooks.
void Initialize();
bool IsLoaded();
bool OwnsRuntime();
void RenderMenu();
} // namespace Sm86
