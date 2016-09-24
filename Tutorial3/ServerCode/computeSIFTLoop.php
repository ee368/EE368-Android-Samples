<?php
#-------------------------------------------------------------------------------
# EE368 Digital Image Processing
# Android Tutorial #3: Server-Client Interaction Example for Image Processing
# Author: Derek Pang (dcypang@stanford.edu), David Chen (dmchen@stanford.edu)
#------------------------------------------------------------------------------

#function for streaming file to client
function streamFile($location, $filename, $mimeType='application/octet-stream')
{ if(!file_exists($location))
  { header ("HTTP/1.0 404 Not Found");
    return;
  }
  
  $size=filesize($location);
  $time=date('r',filemtime($location));
  #html response header
  header('Content-Description: File Transfer');	
  header("Content-Type: $mimeType"); 
  header('Cache-Control: public, must-revalidate, max-age=0');
  header('Pragma: no-cache');  
  header('Accept-Ranges: bytes');
  header('Content-Length:'.($size));
  header("Content-Disposition: inline; filename=$filename");
  header("Content-Transfer-Encoding: binary\n");
  header("Last-Modified: $time");
  header('Connection: close');      

  ob_clean();
  flush();
  readfile($location);
	
}

#**********************************************************
#Main script
#**********************************************************

#<1>set target path for storing photo uploads on the server
$photo_upload_path = "./upload/";
$photo_upload_path = $photo_upload_path.basename( $_FILES['uploadedfile']['name']); 
$photo_upload_indicator_path = "./upload/image_ready";

#<2>set target path for storing result on the server
$downloadFileName = 'processed_';
$processed_photo_output_path = "./output/processed_";
$processed_photo_output_path = $processed_photo_output_path.basename( $_FILES['uploadedfile']['name']); 
$processed_photo_output_indicator_path = "./output/result_ready";
$downloadFileName = $downloadFileName.basename( $_FILES['uploadedfile']['name']); 

#<3>modify maximum allowable file size to 10MB and timeout to 300s
ini_set('upload_max_filesize', '10M');  
ini_set('post_max_size', '10M');  
ini_set('max_input_time', 300);  
ini_set('max_execution_time', 300);  

#<4>Get and stored uploaded photos on the server
if(copy($_FILES['uploadedfile']['tmp_name'], $photo_upload_path)) {
	
	#<5>signal that the image is ready
	$handle = fopen($photo_upload_indicator_path, 'w');
	fprintf($handle, '%s', $photo_upload_path);
	fclose($handle);
	
	#<6>wait until the result is ready
	while (!file_exists($processed_photo_output_indicator_path))
	{
		usleep(1000000);
	}
	usleep(1000000);
	unlink($processed_photo_output_indicator_path);
	
	#<7>stream processed photo to the client
	streamFile($processed_photo_output_path, $downloadFileName,"application/octet-stream");
} else{
    echo "There was an error uploading the file to $photo_upload_path !";
}

?>



