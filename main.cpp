#include <k4a/k4a.h>
#include <opencv2/opencv.hpp>
#include <math.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <string>
#include <vector>
#include <C:\Users\Benutzer\AppData\Local\Programs\Python\Python311\include\Python.h>
#include <windows.h>
#include <cmath>


static std::vector<float> GetHPCoordinates(char** argv, int argc, std::string filename)
{
    std::vector<float> floatArray;

    // 1. Initialisiere den Python-Interpreter
    const char* tempfilename = filename.c_str();

    // 2. Lade das Python-Skript
    PyObject* pName = PyUnicode_DecodeFSDefault("hpd");
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != nullptr) {
        // 3. Hole die Funktion "add" aus dem Modul
        PyObject* pFunc = PyObject_GetAttrString(pModule, "GetPosePoints");

        if (pFunc && PyCallable_Check(pFunc)) {
            // 4. Erstelle Argumente für den Funktionsaufruf
            PyObject* pArgs = PyTuple_Pack(1, PyUnicode_FromString(tempfilename));

            // 5. Rufe die Python-Funktion auf
            PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);

            if (pValue != nullptr)
            {
                if (PyList_Check(pValue) && PyList_Size(pValue) > 0)
                {
                    Py_ssize_t size = PyList_Size(pValue);

                    for (Py_ssize_t i = 0; i < size; ++i)
                    {
                        PyObject* pItem = PyList_GetItem(pValue, i);
                        if (PyFloat_Check(pItem))
                        {
                            float value = static_cast<float>(PyFloat_AsDouble(pItem));
                            floatArray.push_back(value);
                        }
                        else
                        {
                            floatArray.push_back(-1.0);
                            std::cerr << pItem << " is not a float " << i << std::endl;
                        }
                    }
                }
            }
            else {
                PyErr_Print();
                std::cerr << "Call failed" << std::endl;
            }
            Py_XDECREF(pFunc);
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            std::cerr << "Cannot find function 'add'" << std::endl;
        }
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        std::cerr << "Failed to load 'python_script'" << std::endl;
    }

    return floatArray;
}


static void Create_XY_Table(const k4a_calibration_t* calibration, k4a_image_t xy_table)
{
    k4a_float2_t* table_data = (k4a_float2_t*)(void*)k4a_image_get_buffer(xy_table);

    int width = calibration->depth_camera_calibration.resolution_width;
    int height = calibration->depth_camera_calibration.resolution_height;

    std::cout << "Width: " << width << "\nHeight: " << height;


    k4a_float2_t p;
    k4a_float3_t ray;
    int valid;

    for (int y = 0, idx = 0; y < height; y++)
    {
        p.xy.y = (float)y;
        for (int x = 0; x < width; x++, idx++)
        {



            p.xy.x = (float)x;

            k4a_calibration_2d_to_3d(calibration, &p, 1.f, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &ray, &valid);

            if (valid)
            {
                table_data[idx].xy.x = ray.xyz.x;
                table_data[idx].xy.y = ray.xyz.y;
            }
            else
            {
                table_data[idx].xy.x = nanf("");
                table_data[idx].xy.y = nanf("");
            }
        }
    }
}

static k4a_float2_t* GetHumanPoseInPXFromPointCloud(const k4a_calibration_t* calibration, k4a_image_t point_cloud)
{
    int width = calibration->depth_camera_calibration.resolution_width;
    int height = calibration->depth_camera_calibration.resolution_height;
    k4a_float2_t p;
    k4a_float3_t ray;
    int valid;

    k4a_image_t img;
    k4a_float2_t* table_data = (k4a_float2_t*)(void*)k4a_image_get_buffer(img);
    std::vector<float> hpcords;
    k4a_float3_t* point_cloud_data = (k4a_float3_t*)(void*)k4a_image_get_buffer(point_cloud);

    for (int i = 0; i < width * height; i++)
    {

        if (isnan(point_cloud_data[i].xyz.x) || isnan(point_cloud_data[i].xyz.y) || isnan(point_cloud_data[i].xyz.z))
        {
            continue;
        }
        else
        {
            ray.xyz.x = point_cloud_data[i].xyz.x;
            ray.xyz.y = point_cloud_data[i].xyz.y;
            ray.xyz.z = point_cloud_data[i].xyz.z;

            k4a_calibration_3d_to_2d(calibration, &ray, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &p, &valid);
            if (valid)
            {
                table_data[i].xy.x = p.xy.x;
                table_data[i].xy.y = p.xy.y;
            }
            else
            {
                table_data[i].xy.x = nanf("");
                table_data[i].xy.y = nanf("");
            }
        }
    }
    
    return table_data;
}


static void GenerateHPPointCloud(const k4a_image_t depth_image, const k4a_image_t xy_table, k4a_image_t point_cloud, int* hp_point_count)
{
    int width = k4a_image_get_width_pixels(depth_image);
    int height = k4a_image_get_height_pixels(depth_image);

    uint16_t* depth_data = (uint16_t*)(void*)k4a_image_get_buffer(depth_image);
    k4a_float2_t* xy_table_data = (k4a_float2_t*)(void*)k4a_image_get_buffer(xy_table);
    k4a_float3_t* point_cloud_data = (k4a_float3_t*)(void*)k4a_image_get_buffer(point_cloud);

    *hp_point_count = 0;
    for (int i = 0; i < width * height; i++)
    {
        if (depth_data[i] != 0 && !isnan(xy_table_data[i].xy.x) && !isnan(xy_table_data[i].xy.y))
        {
            point_cloud_data[i].xyz.x = xy_table_data[i].xy.x * (float)depth_data[i];
            point_cloud_data[i].xyz.y = xy_table_data[i].xy.y * (float)depth_data[i];
            point_cloud_data[i].xyz.z = (float)depth_data[i];
            (*hp_point_count)++;
        }
        else
        {
            point_cloud_data[i].xyz.x = nanf("");
            point_cloud_data[i].xyz.y = nanf("");
            point_cloud_data[i].xyz.z = nanf("");
        }
    }
}

static void GeneratePointCloud(const k4a_image_t depth_image,
    const k4a_image_t xy_table,
    k4a_image_t point_cloud,
    int* point_count)
{
    int width = k4a_image_get_width_pixels(depth_image);
    int height = k4a_image_get_height_pixels(depth_image);

    uint16_t* depth_data = (uint16_t*)(void*)k4a_image_get_buffer(depth_image);
    k4a_float2_t* xy_table_data = (k4a_float2_t*)(void*)k4a_image_get_buffer(xy_table);
    k4a_float3_t* point_cloud_data = (k4a_float3_t*)(void*)k4a_image_get_buffer(point_cloud);

    *point_count = 0;
    for (int i = 0; i < width * height; i++)
    {
        if (depth_data[i] != 0 && !isnan(xy_table_data[i].xy.x) && !isnan(xy_table_data[i].xy.y))
        {
            point_cloud_data[i].xyz.x = xy_table_data[i].xy.x *(float)depth_data[i];
            point_cloud_data[i].xyz.y = xy_table_data[i].xy.y *(float)depth_data[i];
            point_cloud_data[i].xyz.z = (float)depth_data[i];
            (*point_count)++;
        }
        else
        {
            point_cloud_data[i].xyz.x = nanf("");
            point_cloud_data[i].xyz.y = nanf("");
            point_cloud_data[i].xyz.z = nanf("");
        }
    }
}

static void WritePointCloud(const char* file_name, const k4a_image_t point_cloud, int point_count)
{
    int width = k4a_image_get_width_pixels(point_cloud);
    int height = k4a_image_get_height_pixels(point_cloud);

    k4a_float3_t* point_cloud_data = (k4a_float3_t*)(void*)k4a_image_get_buffer(point_cloud);

    // save to the ply file
    std::ofstream ofs(file_name); // text mode first
    ofs << "ply" << std::endl;
    ofs << "format ascii 1.0" << std::endl;
    ofs << "element vertex"
        << " " << point_count << std::endl;
    ofs << "property float x" << std::endl;
    ofs << "property float y" << std::endl;
    ofs << "property float z" << std::endl;
    ofs << "end_header" << std::endl;
    ofs.close();

    std::stringstream ss;
    for (int i = 0; i < width * height; i++)
    {

        if (isnan(point_cloud_data[i].xyz.x) || isnan(point_cloud_data[i].xyz.y) || isnan(point_cloud_data[i].xyz.z))
        {
            continue;
        }

        ss << (float)point_cloud_data[i].xyz.x << " " << (float)point_cloud_data[i].xyz.y << " "
            << (float)point_cloud_data[i].xyz.z << std::endl;
    }

    std::ofstream ofs_text(file_name, std::ios::out | std::ios::app);
    ofs_text.write(ss.str().c_str(), (std::streamsize)ss.str().length());
}

static std::string SaveColorImage(k4a_image_t image, std::string filename)
{
    filename = "C:\\gewuenschter_Speicherort" + filename;
    int width = k4a_image_get_width_pixels(image);
    int height = k4a_image_get_height_pixels(image);
    cv::Mat img(cv::Size(width, height), CV_8UC4, k4a_image_get_buffer(image), cv::Mat::AUTO_STEP);
    cv::imwrite(filename, img);
    return filename;
}

static std::vector<float> NormalizeHPCoordinates(int ccx, int ccy, int dcx, int dcy, std::vector<float> hpCoordinates)
{
    if (ccx != 0 && ccy != 0 && dcx != 0 && dcy != 0)
    {
        float xFactor = float(dcx) / float(ccx);
        float yFactor = float(dcy) / float(ccy);

        for (int i = 0; i < hpCoordinates.size(); i++)
        {
            if (i % 2 == 1)
            {
                //Y-Koordinaten
                hpCoordinates[i] = hpCoordinates[i] * yFactor;
            }
            else
            {
                //X-Koordinaten
                hpCoordinates[i] = hpCoordinates[i] * xFactor;
            }
        }
    }
    
    return hpCoordinates;
}


static k4a_image_t SkalierePointCloud(const k4a_image_t point_cloud, int point_count)
{
    k4a_image_t scaled = NULL;

    return scaled;
}
// Struktur zur Darstellung eines 2D-Punktes
struct Point {
    float x;
    float y;
};

// Funktion zur Berechnung des Skalarprodukts zweier Vektoren
float dotProduct(const Point& vec1, const Point& vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y;
}

// Funktion zur Berechnung der Länge eines Vektors
float vectorLength(const Point& vec) {
    return std::sqrt(vec.x * vec.x + vec.y * vec.y);
}

// Funktion zur Berechnung des Winkels zwischen zwei Geraden AB und BC
float calculateAngle(const Point& A, const Point& B, const Point& C) {
    Point AB = { B.x - A.x, B.y - A.y };
    Point BC = { C.x - B.x, C.y - B.y };
    float dot = dotProduct(AB, BC);
    float lengthAB = vectorLength(AB);
    float lengthBC = vectorLength(BC);
    // Kosinus des Winkels zwischen den beiden Vektoren
    float cosTheta = dot / (lengthAB * lengthBC);
    // Umwandlung des Kosinuswerts in den Winkel in Grad
    float angle = std::acos(cosTheta) * (180.0 / 3.1415);

    return angle;
}

int main(int argc, char** argv)
{
    int returnCode = 1;
    k4a_device_t device = NULL;
    const int32_t TIMEOUT_IN_MS = 1000;
    k4a_capture_t capture = NULL;
    const std::string colorfilename = "Images\\cimg";
    const std::string depthfilename = "PointClouds\\pc_ascii";
    uint32_t device_count = 0;
    k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    k4a_image_t color_config = NULL;
    k4a_image_t color_image = NULL;
    k4a_image_t depth_image = NULL;
    k4a_image_t xy_table = NULL;
    k4a_image_t point_cloud = NULL;
    int count_depthfiles = 0;
    int count_colorfiles = 0;
    int point_count = 0;

    
    Py_Initialize();

    std::string sourceDir = "C:\\Speicherort_des_Programmcodes";
    SetCurrentDirectoryA(sourceDir.c_str());

    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);
    std::string cwd = buffer;

    PyObject* sysPath = PySys_GetObject("path");
    PyList_Append(sysPath, PyUnicode_DecodeFSDefault(cwd.c_str()));

    std::string tempcolorfilename = "";
    std::string tempdepthfilename = "";

    
    device_count = k4a_device_get_installed_count();

    if (device_count == 0)
    {
        printf("No K4A devices found\n");
        return 0;
    }

    if (K4A_RESULT_SUCCEEDED != k4a_device_open(K4A_DEVICE_DEFAULT, &device))
    {
        printf("Failed to open device\n");
        goto Exit;
    }
    config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    config.color_resolution = K4A_COLOR_RESOLUTION_720P;
    config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
    config.synchronized_images_only = true;
    config.camera_fps = K4A_FRAMES_PER_SECOND_30;

    k4a_calibration_t calibration;
    if (K4A_RESULT_SUCCEEDED !=
        k4a_device_get_calibration(device, config.depth_mode, config.color_resolution, &calibration))
    {
        printf("Failed to get calibration\n");
        goto Exit;
    }
    
    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
        calibration.depth_camera_calibration.resolution_width,
        calibration.depth_camera_calibration.resolution_height,
        calibration.depth_camera_calibration.resolution_width * (int)sizeof(k4a_float2_t),
        &xy_table);

    Create_XY_Table(&calibration, xy_table);

    k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
        calibration.depth_camera_calibration.resolution_width,
        calibration.depth_camera_calibration.resolution_height,
        calibration.depth_camera_calibration.resolution_width * (int)sizeof(k4a_float3_t),
        &point_cloud);

    std::cout<<"\n"<<calibration.depth_camera_calibration.resolution_width<<"\n";
    std::cout<<calibration.depth_camera_calibration.resolution_height<<"\n";

    k4a_device_stop_cameras(device);

    if (K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(device, &config)) {
        std::cerr << "Failed to start Kinect cameras!" << std::endl;
        k4a_device_close(device);
        return -1;
    }
    

    while (true)
    {
    
        if (K4A_WAIT_RESULT_SUCCEEDED == k4a_device_get_capture(device, &capture, K4A_WAIT_INFINITE))
        {
            
            color_image = k4a_capture_get_color_image(capture);
            if (color_image == 0)
            {
                printf("Failed to get color_image from capture\n");
                goto Exit;
            }
            depth_image = k4a_capture_get_depth_image(capture);
            if (depth_image == 0)
            {
                printf("Failed to get depth_image from capture\n");
                goto Exit;
            }
        }
        else
        {
            std::cerr << "Failed to get capture!" << std::endl;
        }
    
        std::vector<float> hpCoordinates;

        tempcolorfilename = colorfilename + std::to_string(count_colorfiles) + ".png";

        hpCoordinates = GetHPCoordinates(argv, argc, SaveColorImage(color_image, tempcolorfilename));
   
        if (!hpCoordinates.size() > 1) {
            goto Exit;
        }
        

        
            // Beispielkoordinaten
            Point linkeSchulter = { hpCoordinates[22], hpCoordinates[23]};
            Point rechteSchulter = { hpCoordinates[24], hpCoordinates[25] };
            Point linkerArm = { hpCoordinates[26], hpCoordinates[27] };
            Point rechterArm = { hpCoordinates[28], hpCoordinates[29] };
            Point linkeHüfte = { hpCoordinates[46], hpCoordinates[47] };
            Point rechteHüfte = { hpCoordinates[48], hpCoordinates[49] };

            float winkelLinkeSchulter = calculateAngle(linkeHüfte, linkerArm, linkeSchulter);
            float winkelRechteSchulter = calculateAngle(rechteHüfte, rechterArm, rechteSchulter);

            // Ausgabe des Ergebnisses
            std::cout << "Der Winkel an der linken Schulter beträgt: " << winkelLinkeSchulter << " Grad" << std::endl;
            std::cout << "Der Winkel an der rechten Schulter beträgt: " << winkelRechteSchulter << " Grad" << std::endl;
      
        

        GeneratePointCloud(depth_image, xy_table, point_cloud, &point_count);

        tempdepthfilename = depthfilename + std::to_string(count_depthfiles) + ".ply";
        WritePointCloud(tempdepthfilename.c_str(), point_cloud, point_count);

        count_colorfiles++;
        count_depthfiles++;

        std::cout<<"Durchlauf: "<<count_colorfiles;
        /*if (count_colorfiles > 5)
        {
            returnCode = 0;
            goto Exit;
        }*/

    }

    k4a_image_release(depth_image);
    k4a_capture_release(capture);
    k4a_image_release(xy_table);
    k4a_image_release(point_cloud);

    returnCode = 0;
Exit:
    if (device != NULL)
    {
        k4a_device_close(device);
        // 7. Beende den Python-Interpreter
        Py_Finalize();
    }

    return returnCode;
}