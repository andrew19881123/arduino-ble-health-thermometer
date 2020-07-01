// Credits nico78 https://forum.arduino.cc/index.php?topic=681472.msg4585031#msg4585031

#include <ArduinoBLE.h>


// define BLE services
BLEService thermometreService("1809");

// define characteristics
BLECharacteristic thermometreLevel("2A1C", BLEIndicate, 5);

/*

  name                        Requirement Read    Write     Notify    Indicate

  Temperature Measurement     Mandatory Excluded  Excluded  Excluded  Mandatory
  Temperature Type            Optional  Mandatory Excluded  Excluded  Excluded
  Intermediate Temperature    Optional  Excluded  Excluded  Mandatory Excluded
  Measurement Interval        Optional  Mandatory Optional  Excluded  Optional
  Measurement Interval        Optional  Mandatory Optional  Excluded  Optional

*/


/*

  Flags                 Mandatory     8bit      0     1   Temperature Units Flag  0   Temperature Measurement Value in units of Celsius
  Flags                 Mandatory     8bit      0     1   Temperature Units Flag  1   Temperature Measurement Value in units of Fahrenheit
  Flags                 Mandatory     8bit      1     1   Time Stamp Flag         0   Time Stamp field not present
  Flags                 Mandatory     8bit      1     1   Time Stamp Flag         1   Time Stamp field present
  Flags                 Mandatory     8bit      2     1   Temperature Type Flag   0   Temperature Type field not present
  Flags                 Mandatory     8bit      2     1   Temperature Type Flag   1   Temperature Type field present
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit
  Flags                 Mandatory     8bit
  Temperature Measurement Value (Celsius)     32bit FLOAT   This field is only included if the flags bit 0 is 0.  C1  FLOAT
  Temperature Measurement Value (Fahrenheit)  32bit FLOAT   This field is only included if the flags bit 0 is 1.  C2  FLOAT
  Time Stamp                                  TIME  FORMAT  If the flags bit 1 is 1 this field is included. If it is 0, this field is not included  C3
  Temperature Type                             8bit UINT8   If the flags bit 2 is set to 1 this field is included. If it is 0, this field is not included C4

  TIME FORMAT (7 octets au total) -> (16bit année) + (8bit mois) + (8bit jour) + (8bit heure) + (8bit minute) + (8bit seconde)
*/

// We will use an array because on a structure, it is possible to have additional bytes added for alignment, only the first 5 bytes interests us.
// Flag(byte->1octet) + Température(Float->4octets)
byte temperatureData[5] = {0,};

void setup() {
  Serial.begin(9600);
  Serial.println("app started");

//   while (!Serial);

  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
  }

  BLE.setLocalName("Thermometer");
  BLE.setAdvertisedService(thermometreService);
  thermometreService.addCharacteristic(thermometreLevel);
  BLE.addService(thermometreService);

  // la valeur de flag sera toujours de 0 (1er octet)
  // le Bit 0 du flag mis à la valeur 0 pour °Celsius, les autres Bits à 0 car on en a pas besoin pour ce test
  // les 4 autres octets qui suivent sont pour l'affichage de la température après conversion dans le bon format
  float temp = 0.0f;
  // conversion du float dans le format approprié
  unsigned long conversionTemp = ieee11073_from_float(temp);
  memcpy(temperatureData + 1, &conversionTemp, 4);
  thermometreLevel.writeValue(temperatureData, 5);

  // Démarrer la publication du service
  BLE.advertise();

  Serial.println("Thermometer published");
  Serial.println("");
}

void loop() {
    // Serial.println("loop");
  static int etat = 0;
  float temperature = -5.0f;
  float temp;
  // Nous sommes un périphérique esclave (serveur)
  // et nous attendons une connexion centrale maitre (client)

  // En attente de connexion d'un client
  BLEDevice central = BLE.central();

  // etat est utilisé pour éviter la répétition de l'information
  if (etat == 0) {
    etat = 1;
    Serial.println("waiting for client to connect");
    Serial.println("");
  }

  // Test si un appareil s'est connecté
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    Serial.println("");

    while (central.connected()) {
      // on incrémente automatiquement la température pour le test
      temperature += 0.1;

      Serial.print("Temp: ");
      Serial.println(temperature);

      // conversion du float dans le format approprié
      unsigned long conversionTemp = ieee11073_from_float(temperature);
    //   unsigned long conversionTemp = temperature;
      memcpy(temperatureData + 1, &conversionTemp, 4);
      thermometreLevel.writeValue(temperatureData, 5);

      // écriture de la valeur dans la caractéristique
      thermometreLevel.writeValue(temperatureData, 5);

      // on change la valeur de la température toutes les 2 secondes
      delay(500);
    }

    Serial.print("disconnected from central: ");
    Serial.println(central.address());
    Serial.println("");
    etat = 0;
  }
}

// Cette fonction n'est pas complète
// mais semble donné un résultat satisfaisant
// pour les données de température
unsigned long ieee11073_from_float(float temperature) {
  uint8_t  exponent = 0xFE; //exponent is -2
  uint32_t mantissa ;

  // ajouté pour température négative
  if (temperature < 0)
  {
    mantissa = (uint32_t)(temperature * 10);
    mantissa |= 0x80000000;
  } else {
    mantissa = (uint32_t)(temperature* 100);
  }

  return ((((uint32_t)exponent) << 24) | mantissa);
}
