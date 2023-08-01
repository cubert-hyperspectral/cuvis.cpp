/** \mainpage CUVIS CPP SDK

The documentation of all functions can be found \ref cuvis.hpp "here".

- - - 

 # Getting started
 
 The cuvis SDK can be used directly as C SDK or via the included wrappers (e.g. cpp, Matlab, Python...), this is the documentation of the CPP wrapper. If you are looking for a pure C API, see \ref cuvis.h "here".
 
 The essentials classes are grouped in the namespace \ref cuvis. This includes all functions to operate the camera and retrieve, process and export images.   \n
 
 Non-essential Helper classes are grouped in the namespace \ref cuvis::aux. Theesere require additional dependencies (e.g. openCV)
 
 # Initialization
 
  The first SDK command should be setting the log level (default: info, \ref cuvis::General::set_log_level). The Programm will start logging into the console. The Debug log is also written to file, for additional informaiton see \ref log.
  
  Then initialize zhe system by setting the location of the settings-directory (default: "C:/Program Files/cuvis/user/settings" for windows or "/etc/cuvis/user/settings" for linux, \ref cuvis::General::init). \n
 From this the required performance settings for the local system are loaded. This step is crucial for recording images. When only processing data offline, this step can be skipped.
 
  With the SDK, several \ref examples are installed.
 Assuming the hardware is set up, perform the following steps to record single measurements in software trigger mode (\ref cuvis::AcquisitionContext::set_operation_mode).\n
 This is equivalent to the Example 05_recordSingleImages_cpp.

 1. load calibration (\ref cuvis::Calibration::Calibration)
 2. load acquisition context (\ref 
cuvis::AcquisitionContext::AcquisitionContext)
 3. set operation mode to Software (\ref cuvis::AcquisitionContext::set_operation_mode)
 4. set camera parameters (e.g. integration time)
 5. set up processing context (\ref 
cuvis::ProcessingContext::ProcessingContext)
 6. prepare processing context arguments: \ref cuvis::ProcessingContext::set_processingArgs	
 7. set up cube exporter (\ref 
cuvis::CubeExporter::CubeExporter)
 8. in a loop run:
    1. capture a measurement (\ref cuvis::AcquisitionContext::capture)
    2. process measurement (\ref  cuvis::ProcessingContext::apply)
    3. read image data (\ref cuvis::Measurement::get_imdata	)
    4. save measurement (\ref cuvis::Exporter::apply)
	
	
 # Usage of C-types
 The CUVIS CPP is a wrapper to the underlying CUVIS C sdk. All handle-based C API calls are replaced by smart pointeres by the wrapper. 
 <b>C and CPP SDK calls cannot be comined!</b>
  
 However several Enumerations and simple data strucutres from the C SDK directly, see \ref typedefs for a full list.
 
 
*/