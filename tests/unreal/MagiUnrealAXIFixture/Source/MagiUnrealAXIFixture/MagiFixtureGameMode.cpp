#include "MagiFixtureGameMode.h"
#include "MagiFixturePawn.h"

AMagiFixtureGameMode::AMagiFixtureGameMode()
{
    DefaultPawnClass = AMagiFixturePawn::StaticClass();
}
