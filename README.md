# Quellcode zur Studienarbeit "Vermessung von Schulterbewegungen mit 3D-Tiefenkameras" von Timo Huter und Felix Steinhauser

## Funktionsweise

Die Main-Datei ist der Kern des Programmes. Wenn man sie ausführt, nimmt die angeschlossene Azure Kinect Kamera Bilder auf und speichert sie im Programmcode-Verzeichnis in den Ordnern "Images" und "PointClouds".
Die hpd.py-Datei wird im Programmcode automatisch aufgerufen und gibt die Schulterpunkte an main zurück. Die Winkel der Schultern im aktuellen Bild werden dann berechnet und laufend über die Konsole ausgegeben.
Um sich die Punktewolke zu visualisieren, sollte man die Ausführung von Main stoppen und in der pc_visualizer.py die gewünschte Datei zum Visualisieren angeben.
Um die Skelettpunkte auf den Farbbildern zu sehen, kann man mit OBS eines der Bilddateien übertragen. Wählt man OBS als Standard-Kamera aus, kann man human_pose_detection.py ausführen. Dieses erkennt dann das Bild in OBS und gibt dieses samt Annotation aus.

Anmerkungen: 
In der Main-Datei sind Dateipfade, die angepasst werden müssen.
- Zeile 11: Muss an den Installationspfad von Python angepasst werden
- Zeile 265: den gewünschten Speicherort für die Farbbilder festlegen
- Zeile 258: Muss den Speichertort des Programmcodes angeben

Ebenso in der pc_visualizer-Datei:
- Zeile 7/8: Dateipfad zu den PointCloud-Dateien
