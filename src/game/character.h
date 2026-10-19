#ifndef SA3DS_GAME_CHARACTER_H
#define SA3DS_GAME_CHARACTER_H

typedef enum { SA3DS_CHARACTER_SONIC = 0 } SA3DSLogicalCharacter;
typedef enum { SA3DS_PRESENTATION_NORMAL = 0, SA3DS_PRESENTATION_PX = 1 } SA3DSPresentationProfile;

typedef struct {
    SA3DSLogicalCharacter logical_character;
    SA3DSPresentationProfile presentation_profile;
} SA3DSCharacterPresentationRequest;

/* Asset resolution intentionally remains outside physics. No PX assets are defined. */

#endif
