# LetsCamp

C++17 / OpenGL 4.1 игра с горско село, герой Toshko, 3D и ортографска камера,
колизии и тъмна вътрешна стая с анимирана запалка.

## Компилиране и стартиране

Нужни са CMake 3.20+, C/C++ компилатор, GLFW, GLM и OpenGL 4.1.
GLAD, stb_image, cgltf и използваната част от meshoptimizer са включени в `external/`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLETSCAMP_BUILD_CHECKS=ON
cmake --build build -j1
ctest --test-dir build --output-on-failure
cd build
./LetsCamp
```

Папката `assets/` се копира до изпълнимия файл. Стартирайте от `build/` или от
корена на проекта, защото пътищата към ресурсите са относителни.

## Управление

- WASD: движение; Shift: спринт.
- Мишка: камера; колелце или V + движение по Y: приближаване/отдалечаване.
- P: пауза и настройки на камерата; Esc: изход.
- Приближаване към вратата на най-малката къща: влизане в стаята.
- Южната врата на стаята: връщане навън. Вътре камерата е само от първо лице.

## Организация

- `Game`: игрова сесия; `Game.cpp`, `GameInput.cpp`, `GameWorld.cpp`, `GameRender.cpp`.
- `letscamp_core`: зареждане и построяване на света, колизии, врати, камера и движение без OpenGL.
- Рисуване: `ModelRenderer`, `InteriorRoom`, `SpriteRenderer`, `PauseMenu`, `Shader`, `Texture`.
- `tests/`: проверки на логиката без графичен контекст.
- `legacy/tile_world/`: запазеният предишен tile-based свят. Проверка на компилацията:
  `cmake -S . -B build -DLETSCAMP_BUILD_LEGACY=ON`.
- `docs/architecture-bg.pdf`: пълно описание на архитектурата на български.

Ортографският 2D режим на камерата продължава да работи в активната 3D сцена.
Старият tile-based свят не се зарежда автоматично и изисква старите си ресурси.

## Поддръжка

Стилът на собствения код е зададен в `.clang-format`. Външните библиотеки не се
преформатират. `Shader`, `Texture` и renderer класовете притежават ресурсите си;
те трябва да се унищожат преди GLFW/OpenGL контекста.

Основни настройки: `include/GameConfig.h`, `include/PlayerDimensions.h`,
`src/Camera.cpp`, `src/ForestWorld.cpp` и `assets/shaders/`.
Създаването на нов свят в момента изисква точно три групи къщи и три групи дървета
в `assets/models/game_asset.glb`. Скелетни анимации и пълен PBR не се поддържат.
