#include "ObstacleClassifier.h"
#include "Phase1_Physical/LogActor.h"
#include "StonePhaseA_Mining/StoneActor.h"
#include "StickPhaseS1_Core/StickActor.h"
#include "Phase4_Construction/DoorActor.h"
EObstacleType UObstacleClassifier::ClassifyObstacle(AActor*O)const{if(!O)return EObstacleType::Passable;if(Cast<ADoorActor>(O))return EObstacleType::Openable;if(Cast<ALogActor>(O)||Cast<AStoneActor>(O))return EObstacleType::Structural;if(Cast<AStickActor>(O))return EObstacleType::Breakable;return EObstacleType::Blocking;}
