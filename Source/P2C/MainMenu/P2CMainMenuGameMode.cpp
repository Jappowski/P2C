#include "P2CMainMenuGameMode.h"
#include "P2CPlayerController.h"

AP2CMainMenuGameMode::AP2CMainMenuGameMode()
{
	PlayerControllerClass = AP2CPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	bUseSeamlessTravel = false;
}
