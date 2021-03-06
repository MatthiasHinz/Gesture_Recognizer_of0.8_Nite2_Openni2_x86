#include "Nite_HandTracker.h";
#include "GUIConnector.h";


Point3f leftHand;
Point3f rightHand;

bool isLeftHandTrackedV, isRightHandTrackedV = false;
int leftHandOutOfSync, rightHandOutOfSync = 0; //counts how many times a hand is out of sync

nite::HandId leftHandId, rightHandId;
const double likelyhood_threshold = 0.6;


nite::Status niteRc;
nite::HandTracker handTracker;
nite::HandTrackerFrameRef handTrackerFrame;

bool printInitMessageLeft = true;
bool printInitMessageRight = true;

//prevents a hand from getting "locked in"

const int maxCount_lockedInCounter = 200;
const int maxCount_outOfSyncCounter = 200;
Point3f last_left_value;
Point3f last_right_value;



Nite_HandTracker::Nite_HandTracker(void)
{
	leftLockedInCounter = 0;
	rightLockedInCounter = 0;
}


Nite_HandTracker::~Nite_HandTracker(void)
{
}


int Nite_HandTracker::initHandTracker(void)
{
	//Signal that there is no right hand tracked yet:
//	BYTE keys [3] = {VK_CONTROL,VK_SHIFT,VK_OEM_COMMA};
//	GUIConnector::sendKeyboardInput(keys, 3);
	
	//Signal that there is no left hand tracked yet:
	//BYTE keys2 [3] = {VK_CONTROL,VK_SHIFT,GUIConnector::L};
	//GUIConnector::sendKeyboardInput(keys2,3);
	try{
		niteRc = nite::NiTE::initialize();
		if (niteRc != nite::STATUS_OK)
		{
			printf("NiTE initialization failed\n");
			return 1;
		}
	}catch(int e){
		printf("NiTE initialization failed\n");
		return e;
	}
	
	try{
		niteRc = handTracker.create();
		if (niteRc != nite::STATUS_OK)
		{
			printf("Couldn't create user tracker\n");
			return 3;
		}
	}catch(int e){
		printf("Couldn't create user tracker, exception code %i\n",e);
		return e;
	}
	
	handTracker.setSmoothingFactor(0.3);
	handTracker.startGestureDetection(nite::GESTURE_WAVE);
	handTracker.startGestureDetection(nite::GESTURE_HAND_RAISE);
	handTracker.startGestureDetection(nite::GESTURE_CLICK);
	return 0;
}

void Nite_HandTracker::updateHandTracker(void)
{
	 //================== OPTAIN DATA ========================
	const nite::Array<nite::HandData>& hands = handTrackerFrame.getHands();
	leftHandOutOfSync++;
	rightHandOutOfSync++;

	bool isLeftHandLost = false;
	bool isRightHandLost = false;

	for (int i = 0; i < hands.getSize(); ++i)
	{
			const nite::HandData& hand = hands[i];
			if (hand.isTracking())
			{
				//printf("%d. (%5.2f, %5.2f, %5.2f)\n", hand.getId(), hand.getPosition().x, hand.getPosition().y, hand.getPosition().z);
				if(hand.getId() == rightHandId){
					rightHand = hand.getPosition();
					rightHandOutOfSync = 0;
					
				}else
					if(hand.getId() == leftHandId){
						leftHand = hand.getPosition();
						leftHandOutOfSync = 0;
					}
			}

			if(hand.getId() == rightHandId){
				//test if hand got stuck
				bool gotStuck = false;
				if(rightLockedInCounter == maxCount_lockedInCounter){
						gotStuck = true;
						printf("Your Right hand got stuck! \n");
						rightLockedInCounter = 0;
					}else{
						if(getRightHandCoordinates().x == last_right_value.x &&
							getRightHandCoordinates().y == last_right_value.y &&
							getRightHandCoordinates().z == last_right_value.z
							){
								rightLockedInCounter++;
						}else{
							rightLockedInCounter=0;
						}
				} 
				
				last_right_value = getRightHandCoordinates();
				//no counting forward or setting 0 because this is done below with the counter

				if(hand.isLost() || !hand.isTracking() || gotStuck){
					printf("Your right hand may get lost!!!!!!!\n State: %s %s %s\n", hand.isLost() ? "isLost":"notLost", hand.isTracking() ? "isTracking":"notTracking", gotStuck ? "gotStuck": "notGotStuck");
					isRightHandLost= true;
				}	
			}
			if(hand.getId() == leftHandId){
				//test if hand got stuck
				bool gotStuck = false;
				bool outOfSync;
				if(leftLockedInCounter == maxCount_lockedInCounter){
						gotStuck = true;
						printf("Your Left hand got stuck! \n");
						leftLockedInCounter = 0;
					}else{
						if(getLeftHandCoordinates().x == last_left_value.x &&
							getLeftHandCoordinates().y == last_left_value.y &&
							getLeftHandCoordinates().z == last_left_value.z
							){
								leftLockedInCounter++;
						}else{
							leftLockedInCounter=0;
						}
				} 
				last_left_value = getLeftHandCoordinates();

				if(hand.isLost()|| !hand.isTracking() || gotStuck){
					printf("Your left hand may get lost! State: %s %s %s\n", hand.isLost() ? "isLost":"notLost", hand.isTracking() ? "isTracking":"notTracking", gotStuck ? "gotStuck": "notGotStuck");
					isLeftHandLost=true;
				}
			}
				// indicate that hand is lost
					//hand.isLost();
					//hand.isNew();
				
		}

	if(isRightHandTrackedV && (isRightHandLost || rightHandOutOfSync > maxCount_outOfSyncCounter)){
			handTracker.stopHandTracking(rightHandId);
			if(isRightHandLost){
				printf("Lost track for right hand!\n");
			}else{
				printf("Lost track for right hand because it is out of sync for too many iterations!\n");
			}

			isRightHandTrackedV = false;
			printInitMessageRight = true;
			rightHandOutOfSync=0;
			//rightHand = Point3f(0,0,0);

			BYTE keys [3] = {VK_CONTROL,VK_SHIFT,VK_OEM_COMMA};
			GUIConnector::sendKeyboardInput(keys, 3);
		}

	if(isLeftHandTrackedV && (isLeftHandLost ||  leftHandOutOfSync > maxCount_outOfSyncCounter)){
			handTracker.stopHandTracking(leftHandId);
			if(isLeftHandLost){
				printf("Lost track for left hand!\n");
			}else{
				printf("Lost track for left hand because it is out of sync for too many iterations!\n");
			}
			isLeftHandTrackedV = false;
			printInitMessageLeft = true;
			leftHandOutOfSync=0;

			BYTE keys [3] = {VK_CONTROL,VK_SHIFT,GUIConnector::L};
			GUIConnector::sendKeyboardInput(keys,3);
		}

		//===== INIT TRACKER IF NEEDED===============

		const nite::Array<nite::GestureData>& gestures = handTrackerFrame.getGestures();

		if(!isRightHandTrackedV && printInitMessageRight){
			printf("\nWave your right hand to start tracking it...\n");
			printInitMessageRight = false; //print only once


		}else if(!isLeftHandTrackedV && isRightHandTrackedV && printInitMessageLeft){
			printf("\nWave your left hand to start tracking it...\n");
			printInitMessageLeft = false;
		}

	    //check whether readframe works
		try{
			niteRc = handTracker.readFrame(&handTrackerFrame);
			if (niteRc != nite::STATUS_OK){
				printf("Get next frame failed\n");
				return;
			}
		}catch(int e){
			printf("Get next frame failed with an exception. Code: %i\n", e);
			return;
		}


		for (int i = 0; i < gestures.getSize() ; ++i)
		{
			if (gestures[i].isComplete() ){
				if(gestures[i].getType()==nite::GESTURE_CLICK)
				{
					printf("\nGESTURE CLICK" );

					//Zoom in // zoom out
					if(GetKeyState(VK_LBUTTON) >= 0){
						GUIConnector::clickLeftMouseButton();

					}else{
						GUIConnector::releaseLeftMouseButton();
						GUIConnector::clickLeftMouseButton();
						GUIConnector::clickLeftMouseButton();
					}


					/*if(GetKeyState(VK_LBUTTON) >= 0){
						BYTE keys[1] = {VK_OEM_PLUS};
						GUIConnector::sendKeyboardInput(keys,1);
						printf(" (\"+\"-Key pressed / Zoom in), Left button state: %i\n",GetKeyState(VK_LBUTTON));
					}else{
						BYTE keys[1] = {VK_OEM_MINUS};
						GUIConnector::sendKeyboardInput(keys,1);
						printf(" (\"-\"-Key pressed / Zoom out), Left button state: %i\n",GetKeyState(VK_LBUTTON));
					} */

				}

				/*if(gestures[i].getType()==nite::GESTURE_HAND_RAISE)
				{
					printf("\nGESTURE HAND RAISE\n" );
				}*/
				
				if(gestures[i].getType()==nite::GESTURE_WAVE)
				{
					printf("\nGESTURE WAVE\n" );
				}
						
			}
				
			

			if (gestures[i].isComplete() && gestures[i].getType() == nite::GESTURE_WAVE && (!isRightHandTrackedV || !isLeftHandTrackedV)
				&& gestures[i].getCurrentPosition()!= leftHand && gestures[i].getCurrentPosition() !=rightHand)
			{
				nite::HandId newId;
				handTracker.startHandTracking(gestures[i].getCurrentPosition(), &newId);
				nite::Point3f start = gestures[i].getCurrentPosition();
				bool foundHand = false;
				if(!isRightHandTrackedV) {
					rightHandId = newId;
					isRightHandTrackedV=true;
					rightHandOutOfSync=0;
					foundHand = true;
					printf("Found right hand!");
					BYTE keys [3] = {VK_CONTROL,VK_SHIFT,VK_F1};
					GUIConnector::sendKeyboardInput(keys,3);

				}else if(!isLeftHandTrackedV){
					leftHandId = newId;
					isLeftHandTrackedV=true;
					leftHandOutOfSync=0;
					foundHand = true;
					printf("Found left hand!");
					BYTE keys [3] = {VK_CONTROL,VK_SHIFT,GUIConnector::K};
					GUIConnector::sendKeyboardInput(keys,3);
				}
				if(foundHand){

				}
			}
		}


}

Point3f Nite_HandTracker::getLeftHandCoordinates(void)
{
	return leftHand;
}



Point3f Nite_HandTracker::getRightHandCoordinates(void)
{
	return rightHand;
}


//bool Nite_HandTracker::isLeftHandRecognized(void)
//{
//	return false;
//}


bool Nite_HandTracker::isRightHandTracked(void)
{
	return isRightHandTrackedV;
}


bool Nite_HandTracker::isLeftHandTracked(void)
{
	return isLeftHandTrackedV;
}

void Nite_HandTracker::initLeftHand(Point3f position){
	nite::HandId newId;
	handTracker.startHandTracking(position, &newId);



	leftHandId = newId;
	isLeftHandTrackedV=true;
	leftHandOutOfSync=0;
	printf("Found left hand!");
	BYTE keys [3] = {VK_CONTROL,VK_SHIFT,GUIConnector::K};
	GUIConnector::sendKeyboardInput(keys,3);
	
}

void Nite_HandTracker::initRightHand(Point3f position){
	nite::HandId newId;
	handTracker.startHandTracking(position, &newId);
	rightHandId = newId;
	isRightHandTrackedV=true;
	rightHandOutOfSync=0;
	printf("Found right hand!");
	BYTE keys [3] = {VK_CONTROL,VK_SHIFT,VK_F1};
	GUIConnector::sendKeyboardInput(keys,3);
}

/*
			const nite::Array<nite::GestureData>& gestures = handTrackerFrame.getGestures();

		//these are onhanded gestures
		for (int i = 0; i < gestures.getSize() && (!isLeftHandTracked || !isRightHandTrackedV); ++i)
		{

			if (gestures[i].isComplete())
			{
				if(gestures[i].getType() == nite::GESTURE_CLICK){
					return "CLICK recognized";
				}

				if(gestures[i].getType() == nite::GESTURE_HAND_RAISE){
					return "HAND RAISE recognized";
				}

				if(gestures[i].getType() == nite::GESTURE_WAVE){
					return "WAVING recognized";
				}
					
			}
		}
*/







					/*
					INPUT input = {0};
					//ZeroMemory(&input, sizeof(input));
					input.type = INPUT_MOUSE;
					input.mi.dwFlags = MOUSEEVENTF_WHEEL | MOUSEEVENTF_VIRTUALDESK;
					if(GetKeyState(VK_LBUTTON) >= 0){
						input.mi.mouseData = 5;
						printf(" (Mousewheel +5 / Zoom in), Left button state: %i\n",GetKeyState(VK_LBUTTON));
					}else{
						input.mi.mouseData = -5;
						printf(" (Mousewheel -5 / Zoom out), Left button state: %i\n",GetKeyState(VK_LBUTTON));
					} 

					int returnCode=SendInput(1, &input, sizeof(input));
					*/


			/* 
			INPUT input = {0};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = VK_CONTROL;
			input.ki.dwFlags = 0; // KEYS DOWN!!!
			int returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = VK_SHIFT;
			returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = 0x4C; //l-key
			returnCode=SendInput(1, &input, sizeof(input));
			
			input.ki.dwFlags = KEYEVENTF_KEYUP; //KEYS UP!!!!
			
			returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = VK_SHIFT;
			returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = VK_CONTROL;
			returnCode=SendInput(1, &input, sizeof(input));*/

			/*
			INPUT input = {0};
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = VK_CONTROL;
			input.ki.dwFlags = 0; // KEYS DOWN!!!
			int returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = VK_SHIFT;
			returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = 0x4C; //l-key
			returnCode=SendInput(1, &input, sizeof(input));
			
			input.ki.dwFlags = KEYEVENTF_KEYUP; //KEYS UP!!!!
			
			returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = VK_SHIFT;
			returnCode=SendInput(1, &input, sizeof(input));

			input.ki.wVk = VK_CONTROL;
			returnCode=SendInput(1, &input, sizeof(input));*/