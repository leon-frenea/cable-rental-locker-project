#include "stm32f10x.h"
#include "bsp.h"
#include "utils.h"
#include "rfid.h"
#include "cable.h"
#include "adxl.h"
#include "servo.h"
#include "uart.h"

// --- DEFINITION DU PROTOCOLE ---
#define START_MARKER 0xFF
#define END_MARKER   0x55

// Etats du système
typedef enum {
    STATE_LOCKED,
    STATE_OPEN_AND_BUSY
} SystemState;

// Variables Globales
SystemState current_state = STATE_LOCKED;
uint64_t current_user_id = 0;


// =============================================================
//               GESTION DES UTILISATEURS
// =============================================================

// Liste des badges autorisés (Format 64 bits obligatoire pour tes ID)
const uint64_t AUTHORIZED_BADGES[] = {
    0x31400eb1f8, 
    0x5000051991,
    0x30c0056df4,
		0x50000518c0
    // Ajoute autant de lignes que tu veux ici...
};

// Macro automatique pour calculer le nombre de badges dans le tableau
#define BADGE_COUNT (sizeof(AUTHORIZED_BADGES) / sizeof(AUTHORIZED_BADGES[0]))

// Fonction simple pour vérifier si un ID est dans la liste
uint8_t Is_Badge_Authorized(uint64_t id_to_check) {
    for (int i = 0; i < BADGE_COUNT; i++) {
        if (AUTHORIZED_BADGES[i] == id_to_check) {
            return 1; // TROUVÉ !
        }
    }
    return 0; // PAS TROUVÉ
}

// Fonction pour envoyer le paquet formaté pour Python
// Ordre : [START] [ID 4o] [IS_OPEN 1o] [BNC 1o] [SONDE 1o] [BANANE 1o] [END]
void Send_Inventory_Packet(uint64_t id, uint8_t is_opening, Inventory_t *inv) {
    uint8_t packet[10];
    
    packet[0] = START_MARKER;
    
    // ID sur 4 octets
    packet[1] = (id >> 24) & 0xFF;
    packet[2] = (id >> 16) & 0xFF;
    packet[3] = (id >> 8)  & 0xFF;
    packet[4] = (id)       & 0xFF;
    
    packet[5] = is_opening; // 1 = Ouverture, 0 = Fermeture, 2 = TENTATIVE DE VOL
    
    // On remplit avec les compteurs calculés par Cable_Perform_Inventory
    packet[6] = inv->count_bnc;
    packet[7] = inv->count_sonde;
    packet[8] = inv->count_banane;
    
    packet[9] = END_MARKER;
    
    UART2_SendBytes(packet, 10);
}

int main(void) {
    Inventory_t current_inventory;
    uint64_t scanned_id = 0;

    SystemInit();
    
    // --- INIT ---
    UART2_Init();
    IO_Config_Buzzer_Button();
    Servo_Init();
    ADXL_Init();
    Cable_Init(); // Configure le Mux et TIM3
    RFID_Init();  // Lance le 125kHz

    // Beep de démarrage
    Buzzer_Beep(100); Delay_us(100000); Buzzer_Beep(100);

    while (1) {
        
        switch (current_state) {
            
            // --------------------------------------------------------
            // CAS 1 : ARMOIRE VERROUILLÉE
            // --------------------------------------------------------
            case STATE_LOCKED:
                
                // A. Surveillance RFID (Semi-Bloquant)
                // - Si pas de badge : retourne 0 en ~5ms
                // - Si badge : lit l'ID en ~100ms et retourne 1
                if (RFID_CheckData(&scanned_id)) {
                    
                    if (Is_Badge_Authorized(scanned_id)) {
                        // 1. Accès Autorisé
                        Buzzer_Beep(50); 
                        current_user_id = scanned_id;
                        
                        // 2. Ouverture Porte
                        Servo_Open();
                        
                        // 3. INVENTAIRE "AVANT"
                        // Petit délai pour laisser le servo bouger
                        Delay_us(500000); 
                        // Scan intelligent (moyenne + seuils)
                        Cable_Perform_Inventory(&current_inventory);
                        
                        // 4. Envoi UART vers Python (Ouverture)
                        Send_Inventory_Packet(current_user_id, 1, &current_inventory);
                        
                        // 5. Changement d'état
                        current_state = STATE_OPEN_AND_BUSY;
                    }
                }
                
                // B. Surveillance VOL (ADXL)
                // Exécuté 99% du temps (quand RFID_CheckData retourne 0)
                else {
                    if (ADXL_Detect_Theft()) {
                        // ALARME !
												Send_Inventory_Packet(current_user_id, 2, &current_inventory);

                        for(int i=0; i<5; i++) {
                            Buzzer_Beep(100);
                            Delay_us(100000);
                        }
                    }
                }
                break;

            // --------------------------------------------------------
            // CAS 2 : ARMOIRE OUVERTE (Session Utilisateur)
            // --------------------------------------------------------
            case STATE_OPEN_AND_BUSY:
                
                // On attend que l'utilisateur appuie sur le bouton pour rendre le matos
                if (Button_Read()) {
                    
                    // 1. Débounce sommaire
                    Delay_us(50000); 
                    
                    // 2. Fermeture Porte
                    Servo_Close();
                    
                    // 3. Bip de confirmation
                    Buzzer_Beep(100);
                    
                    // 4. INVENTAIRE "APRES"
                    Delay_us(500000); 
                    Cable_Perform_Inventory(&current_inventory);
                    
                    // 5. Envoi UART vers Python (Fermeture)
                    Send_Inventory_Packet(current_user_id, 0, &current_inventory);
                    
                    // 6. Retour état verrouillé
                    current_user_id = 0;
                    current_state = STATE_LOCKED;
                }
                break;
        }

        // Petit délai pour stabiliser la boucle principale
        Delay_us(10000); 
    }
}