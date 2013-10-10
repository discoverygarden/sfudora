<?php

/**
 * @file
 * Ajaxplorer callback stuff.
 */

/**
 * The callback proper.
 *
 * A few $_GET parameters we care about:
 * - is_dir: Whether the file to we're ingesting is actually a direcotry of
 *   files.
 * - download_base_url: Where to hit Ajaxplorer.
 * - download_query_params: URL-encoded parameters to throw back at Ajaxplorer
 *   when we go to download whatever resource. We'll parse 'em into an
 *   associative array so they're easier to use later.
 *
 * @return array
 *   A form, 'cause forms are awesome.
 */
function sfudora_ajaxplorer_ingest() {
  $params = drupal_get_query_parameters();

  // Parse download_query_params into an associative array.
  $qp = array();
  parse_str($params['download_query_params'], $qp);
  $params['download_query_params'] = $qp;

  // XXX: Start content region only hack. Hide ALL elements, except those
  // related to the content... Try to prevent people from hitting other links on
  // the site.
  $js = <<<EOJS
  jQuery(document).ready(function() {
    to_show = jQuery('#block-system-main *, #messages *').parentsUntil('body').andSelf();
    jQuery('body *').not(to_show).hide();
    jQuery('body').removeClass('admin-menu');
  });
EOJS;
  $css = <<<EOCSS
.page-sfudora-ingest #admin-menu {
  display: none;
}
EOCSS;
  drupal_add_js($js, array(
    'type' => 'inline',
    'scope' => 'footer',
    'preprocess' => FALSE,
  ));
  drupal_add_css($css, array(
    'type' => 'inline',
    'preprocess' => FALSE,
  ));
  // XXX: End content region only hack.
  return drupal_get_form('sfudora_ajaxplorer_ingest_form', $params);
}

/**
 * Form building function.
 */
function sfudora_ajaxplorer_ingest_form($form, &$form_state, $params) {
  form_load_include($form_state, 'inc', 'sfudora', 'includes/ajaxplorer_ingest');

  $download_url = url($params['download_base_url'], array(
    'query' => $params['download_query_params'],
    'absolute' => TRUE,
  ));

  $form_state['storage']['download_url'] = $download_url;
  $form_state['storage']['ajaxplorer_params'] = $params;

  $form_name = "Basic image MODS form";
  $form = xml_form_builder_get_form($form, $form_state, $form_name);

  $form['restrict_options'] = array(
    '#type' => 'radios',
    '#title' => t("Is this publication restricted?"),
    '#default_value' => 'not_restricted',
    '#options' => array(
      'not_restricted' => t("No restriction"),
      'embargo' => t('Use embargo'),
      'xacml' => t('Use XACML'),
    ),
  );

  $form['embargo_duration'] = array(
    '#type' => 'radios',
    '#tree' => TRUE,
    '#title' => t('Duration of Embargo'),
    '#default_value' => 'indefinite',
    '#options' => array(
      'indefinite' => t('Embargo indefinitely (must be lifted manually)'),
      'finite' => t('Embargo until date specified'),
    ),
    '#states' => array(
      'visible' => array(
        ':input[name=restrict_options]' => array('value' => 'embargo'),
      ),
    ),
    '#default_value' => 'indefinite',
  );
  $form['embargo_date'] = array(
    '#type' => 'datepicker',
    '#title' => t('Embargo date'),
    '#description' => t('The date until which this item should be embargoed (we will check with you on this date to confirm that it is ready for release)'),
    '#states' => array(
      'visible' => array(
        ':input[name=restrict_options]' => array('value' => 'embargo'),
        ':input[name=embargo_duration]' => array('value' => 'finite'),
      ),
    ),
    '#default_value' => FALSE,
  );

  // Choose xacml.
  $xacml_path = drupal_get_path('module', 'sfudora') . SFUDORA_XACML_FOLDER;
  $xacmls = file_scan_directory($xacml_path, '/.*\.xml$/');

  $options = array();
  foreach ($xacmls as $xacml) {
    $value = $xacml->name;
    $key = drupal_realpath($xacml->uri);
    $options[$key] = $value;
  }

  $form['xacml'] = array(
    '#type' => 'select',
    '#title' => t('Select XACML file'),
    '#options' => $options,
    '#states' => array(
      'visible' => array(
        ':input[name=restrict_options]' => array('value' => 'xacml'),
      ),
    ),
  );
  
  // Set a specific callback for the submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  if ($params['is_dir'] == 'true') {
    sfudora_ajaxplorer_folder_ingest_form($form, $form_state, $params);
  }
  
  return $form;
}

/**
 * Hide the title of object ingest form.
 * 
 * @param array $params
 *   The parameters that ajaxplorer passed.
 */
function sfudora_ajaxplorer_folder_ingest_form(&$form, &$form_state, $params) {
  $file = $params['download_query_params']['file'];
  $path = explode('/', $file);
  $name = end($path);
  $form['titleInfo']['title']['#type'] = 'hidden';
  $form['titleInfo']['title']['#value'] = $name;
}


/**
 * Form validation function.
 */
function sfudora_ajaxplorer_ingest_form_validate(&$form, &$form_state) {
  // TODO: Do the XML Form validation stuffs...
  $date = $form_state['values']['embargo_date'];
  $time_now = time();
  $parsed = strtotime($date);
  if ($form_state['values']['restrict_options'] == 'embargo' && $form_state['values']['embargo_duration'] == 'finite') {
    if ($parsed <= $time_now) {
      form_set_error('embargo_date', t('You cannot specify a date in the past!'));
      $newembargo = array();
      foreach ($form['embargo'] as $key => $value) {
        $newembargo[$key] = $value;
        if ($key == 'embargo_date') {
          $newembargo['error_message'] = array(
            "#markup" => t('You cannot specify a date in the past!'),
          );
        }
      }
      $form['embargo'] = $newembargo;
    }

  }
}

/**
 * Form submission function.
 */
function sfudora_ajaxplorer_ingest_form_submit(&$form, &$form_state) {

  // Get the ajaxplorer params.
  $params = $form_state['storage']['ajaxplorer_params'];
  $download_url = "";
  if (isset($form_state['storage']['download_url'])) {
    $download_url = $form_state['storage']['download_url'];
  }
  else {
    drupal_set_message(t("Download Url not exist"), "Error");
  }
  
  $batch = array(
      'operations' => array(
        array('sfudora_ajaxplorer_download_file_data', array($params, $download_url)),
        array('sfudora_ajaxplorer_ingest_object_batch',
          array($form, $form_state)),
      ),
      'title' => t('SFU Ingest'),
      'init_message' => t('Ingest from Ajaxplorer'),
      'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaning @estimate.'),
      'error_message' => t('An error has occurred.'),
      'file' => drupal_get_path('module', 'sfudora') . '/includes/ajaxplorer_ingest.inc',
      'finished' => 'sfudora_ajaxplorer_ingest_object_finished',
    );
    batch_set($batch);
    batch_process("/islandora/object/islandora:root");
 
  

}

/**
 * The batch process of ingest object.
 */
function sfudora_ajaxplorer_ingest_object_batch($form, $form_state, &$context) {
  $files = $context['results']['files'];
  $result = &$context['results'];
  $sandbox = &$context['sandbox'];
  if (!isset($sandbox['offset'])) {
    $sandbox['offset'] = 0;
    $sandbox['total'] = count($files);
    if ($sandbox['total'] === 0) {
      return;
    }
  }
  $context['message'] = t('Processing results @start to @end.', array(
    '@start' => $sandbox['offset'],
    '@end' => $sandbox['total'],
  ));
  $form_state['storage']['ajaxplorer_params']['is_dir'] = "false";
  $offset = $sandbox['offset'];
  
  $pid = sfudora_ajaxplorer_ingest_single_object($form, $form_state, $files[$offset], $result);
  $result['islandora_batch_ingest_object'][$files[$offset]->ajaxplorer_file_path] = $pid;
  $sandbox['offset']++;
  
  if ($sandbox['offset'] >= $sandbox['total']) {
    file_unmanaged_delete($context['results']['files']);
    file_unmanaged_delete_recursive($context['results']['temp_zip']);
  }
  $context['finished'] = $sandbox['offset'] / $sandbox['total'];
}

/**
 * Batch finish function.
 */
function sfudora_ajaxplorer_ingest_object_finished($success, $results, $operations) {
  if ($success) {
    drupal_goto('islandora/object/islandora:root');
  }
}

/**
 * Prepair a single object and add it to the new object array.
 * 
 * @param string $parent_collection
 *   The collection that the object should be ingested into.
 * 
 * @return string  
 *   The  new object pid.
 */
function sfudora_ajaxplorer_ingest_single_object(&$form, &$form_state, $file_info, $result) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  
  $parent_collection = variable_get('sfudora_repository_pid', 'islandora:root');
  if (isset($file_info->parent_collection) && $file_info->parent_collection != $parent_collection ) {
    $parent_collection = $result['islandora_batch_ingest_object'][$file_info->parent_collection];
  }
  

  $title = isset($file_info->file_name) ? $file_info->file_name : $form_state['values']['titleInfo']['title'];
  $file_info->file_name = isset($file_info->file_name) ? $file_info->file_name : $file_info->ajaxplorer_file_path;
  
  $content_model = isset($file_info->content_model) ? $file_info->content_model : sfudora_ajaxplorer_get_content_model($file_info->file_name);
  $new_object = islandora_prepare_new_object("sfu", $title, array(), array($content_model), array(array('relationship' => 'isMemberOfCollection', 'pid' => $parent_collection)));
  
  if (isset($file_info->is_file) && $file_info->is_file == TRUE) {
    
    sfudora_ajaxplorer_ingest_create_obj_datastream($form, $form_state, $new_object, $file_info);
    // Create the DDI datastream.
    sfudora_ajaxplorer_ingest_create_ddi_datastream($form, $form_state, $new_object);
  }
  
    // Add embargo to this object.
  if (isset($form_state['values']['restrict_options']) &&
      $form_state['values']['restrict_options'] == 'embargo') {
    if (isset($form_state['values']['embargo_date']) &&
        isset($form_state['values']['embargo_duration']) &&
        $form_state['values']['embargo_duration'] == 'finite') {
      $embargo_date = $form_state['values']['embargo_date'];
    }
    else {
      $embargo_date = "indefinite";
    }
    sfudora_ajaxplorer_ingest_set_embargo(array($new_object), $embargo_date);
  }

  if (isset($form_state['values']['restrict_options']) &&
      $form_state['values']['restrict_options'] == 'xacml') {
    sfudora_ajaxplorer_ingest_set_xacml($new_object, $form_state['values']['xacml']);
  }
  
  $repo_id = $form_state['storage']['ajaxplorer_params']['repo_id'];
  sfudora_ajaxplorer_add_file_object_relationship($file_info->ajaxplorer_file_path, $repo_id, $new_object);
  
  islandora_add_object($new_object);
  return $new_object->id;
}

/**
 * Creates a DDI Datastream.
 *
 * @param array $form
 *   The form.
 * @param string $form_state
 *   The form state.
 * @param object $new_object
 *   The new Object.
 */
function sfudora_ajaxplorer_ingest_create_ddi_datastream(&$form, &$form_state, $new_object) {

  // Make copies of the form and form_state.
  $ddi_form = $form;
  $ddi_form_state = $form_state;

  // All of the form values that are not part of the form builder form will be
  // removed for processing.
  unset($ddi_form['restrict_options']);
  unset($ddi_form['embargo_duration']);
  unset($ddi_form['embargo_date']);
  unset($ddi_form['xacml']);
  unset($ddi_form_state['values']['restrict_options']);
  unset($ddi_form_state['values']['embargo_duration']);
  unset($ddi_form_state['values']['embargo_date']);
  unset($ddi_form_state['values']['xacml']);

  // Create the DDI datastream.
  $xml_form = new XMLForm($ddi_form_state);
  $ddi_doc = $xml_form->submit($ddi_form, $ddi_form_state);
  $ddi_ds = $new_object->constructDatastream('DDI', 'M');
  $ddi_ds->label = 'DDI Record';
  $ddi_ds->mimeType = 'text/xml';
  $ddi_ds->setContentFromString($ddi_doc->document->saveXML());
  $new_object->ingestDatastream($ddi_ds);
}

/**
 * Creates a OBJ Datastream.
 * 
 * @param array $form
 *   The form.
 * @param string $form_state
 *   The form state.
 * @param object $new_object
 *   The new Object.
 * 
 * @return string
 *   Returns the temporary file path of the downloaded file.
 */
function sfudora_ajaxplorer_ingest_create_obj_datastream(&$form, &$form_state, $new_object, $file_info) {
  module_load_include('inc', 'islandora', 'includes/mime_detect');

  $mime_detect = new MimeDetect();
  $obj_mimetype = $mime_detect->getMimetype($file_info->file_name);

  // Create the OBJ datastream.
  $obj_ds = $new_object->constructDatastream('OBJ', 'M');
  $obj_ds->label = $file_info->file_name;
  $obj_ds->mimeType = $obj_mimetype;
  $obj_ds->setContentFromFile($file_info->uri, FALSE);
  $new_object->ingestDatastream($obj_ds);

  return $temp_file_path;

}

/**
 * Set embargo of the object.
 * 
 * @param AbstractObject|string $objects
 *   The object or the pid of the object.
 * @param string $end
 *   The expire date (yyyy-mm-dd).
 */
function sfudora_ajaxplorer_ingest_set_embargo($objects, $end = "indefinite") {
  module_load_include('inc', 'islandora_scholar_embargo');
  foreach ($objects as $object) {
    islandora_scholar_embargo_set_embargo($object, NULL, $end);
  }
}

/**
 * Add the xacml to an object.
 * 
 * @param string|AbstractObject $objects
 *   The object (or its pid) that the xacml should be added to
 * @param string $xacml
 *   The real path pf the xacml file.
 */
function sfudora_ajaxplorer_ingest_set_xacml($objects, $xacml) {
  foreach ($objects as $object) {
    if (is_string($object)) {
      $object = islandora_object_load($object);
    }
    $xacml_ds = $object->constructDatastream('POLICY', 'X');
    $xacml_ds->setContentFromFile($xacml);
    $xacml_ds->label = "XACML Policy Stream";
    if ($object->getDatastream('POLICY')) {
      $object->purgeDatastream('POLICY');
    }
    $object->ingestDatastream($xacml_ds);
  }
}

/**
 * Get the data from a file using a url.
 * 
 * @param AbstractObject|string $download_url
 *   The url of the file to load.
 * 
 * @returns string $temp_file_path
 *   The path of the file in the tmp directory.
 */
function sfudora_ajaxplorer_download_file_data($params, $download_url, &$context) {
  $context['message'] = t('Downloading Content.');
  // Get the path info of the download_url
  $download_url_path_info = pathInfo($download_url);
  // Check to see if the file is a zip file.
  $is_zip_file = $is_single_file = isset($download_url_path_info['extension']) ? FALSE : TRUE;

  
  // We want to pass the cookie information.
  $temparray = array();
  foreach ($_COOKIE as $key => $value) {
    $temparray[] = $key . "=" . $value;
  }
  $temparray[] = 'path:/';
  $cookies = implode('; ', $temparray);

  $temp_file_name = 'ajaxplorer_temp';
  $temp_file_path = file_create_filename($temp_file_name, 'temporary://');
  $fp = fopen($temp_file_path, 'w+');

  $ch = curl_init($download_url);
  curl_setopt($ch, CURLOPT_COOKIE, $cookies);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
  curl_setopt($ch, CURLOPT_TIMEOUT, 50);
  curl_setopt($ch, CURLOPT_FILE, $fp);
  curl_setopt($ch, CURLOPT_FOLLOWLOCATION, TRUE);
  curl_exec($ch);
  curl_close($ch);
  fclose($fp);
  
  if ($is_zip_file) {
    //creates file and collection arrays, storing in the context results
    sfudora_ajaxplorer_get_folder_data($temp_file_path, $context);
  }
  else {
    sfudora_ajaxplorer_get_file_data($params, $temp_file_path, $context); 
  }
    
}

/**
 * Get the folder information.
 * 
 * @param string $download_url
 *   The download url in ajaxplorer.
 */
function sfudora_ajaxplorer_get_folder_data($file_data, &$context) {
  
  $zip = new ZipArchive();
  $zip_file = drupal_realpath($file_data);
  if (($error = $zip->open($zip_file)) !== TRUE) {
    return FALSE;
  }

  $count = $zip->numFiles;
  $root_collection = variable_get('sfudora_repository_pid', 'islandora:root');
  
  $files_info = array();
  $collections_info = array();
  
  for ($i = 0; $i < $count; $i++) {
    $name = $zip->getNameIndex($i);
    $path_info = pathinfo($name);

    $file_entry = new stdClass();
    $file_entry->parent_collection = $path_info['dirname'] . '/';
    $file_entry->ajaxplorer_file_path = $name;
    $file_entry->is_file = TRUE;
    $file_entry->file_name = $path_info['basename'];
    
    if (isset($path_info['extension']) && strpos($path_info['dirname'], '__MACOSX') === FALSE && $path_info['extension'] != 'DS_Store') {    
      $file_entry->uri = "zip://$zip_file#$name";
      $files_info[] = $file_entry;
    }
    else {
      $file_entry->parent_collection = $file_entry->parent_collection  == "./" ? $root_collection : $file_entry->parent_collection ;
      $file_entry->content_model = "islandora:collectionCModel";
      $collections_info[] = $file_entry;
    }
    
  }

  $files_info = array_merge($collections_info, $files_info);
  
  $zip->close();

  $context['results']['files'] = $files_info;
  $context['results']['temp_zip'] = $zip_file;
}

/**
 * Get the folder information.
 * 
 * @param string $download_url
 *   The download url in ajaxplorer.
 */
    
function sfudora_ajaxplorer_get_file_data($params, $file_path, &$context) {

  $file = drupal_realpath($file_path);
  
  $root_collection = variable_get('sfudora_repository_pid', 'islandora:root');
  
  $files_info = array();
  $path_info = pathinfo($file);
  
  $file_entry = new stdClass();
  if (isset($path_info['extension']) && strpos($path_info['dirname'], '__MACOSX') === FALSE && $path_info['extension'] != 'DS_Store') {

    $file_entry->parent_collection = $root_collection;
    $file_entry->ajaxplorer_file_path = $params['download_query_params']['file'];
    $file_entry->is_file = TRUE;
    $files_info[] = $file_entry;
  }
 
  $context['results']['files'] = $files_info;
  $context['results']['temp_zip'] = $file;
}



/**
 * Add relationship between ajaxplorer file and islandora object.
 * 
 * @param string $file_path
 *   The path of the file in ajaxplorer.
 * @param string $repo_id
 *   The repository Id of the file on ajaxplorer.
 * @param FedoraObject $object
 *   The object that should add relationship
 */
function sfudora_ajaxplorer_add_file_object_relationship($file_path, $repo_id, $object) {

  $object->relationships->registerNamespace("ajxp", SFUDORA_AJXP_URI);

  $object->relationships->add(SFUDORA_AJXP_URI, "file", $file_path, TRUE);

  $object->relationships->add(SFUDORA_AJXP_URI, "repository", $repo_id, TRUE);
}

/**
 * Check to see if the objects exists in fedora.
 * 
 * @param string $file_path
 *   The path of the file in ajaxplorer.
 * @param string $repo_id
 *   The repository Id of the file on ajaxplorer.
 * 
 * @return array
 *   Returns the result form the query.
 */
function sfudora_ajaxplorer_object_check($file_path, $repo_id) {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $repo = islandora_get_tuque_connection();

  $query = "
    SELECT ?pid
    FROM <#ri>
    WHERE 
    {
      ?pid <http://ajaxplorer.infofile> \"$file_path\" ; 
      <http://ajaxplorer.inforepository> \"$repo_id\".
    }";

  return $repo->repository->ri->sparqlQuery($query);

}

/**
 * Function to return the content model based on mimetype.
 * 
 * @param string $file
 *   The file.
 *
 * @return string
 *   Returns the content model to be used.
 */
function sfudora_ajaxplorer_get_content_model($file) {
  module_load_include('inc', 'islandora', 'includes/mime_detect');

  // Get the mimetype of the file.
  $mime_detect = new MimeDetect();
  $mimetype = $mime_detect->getMimetype($file);

  // Get all of the enabled solution packs.
  $connection = islandora_get_tuque_connection();
  $enabled_solution_packs = module_invoke_all('islandora_required_objects', $connection);

  // Set generic default content model.
  $content_model = "sfu:genericCModel";

  switch ($mimetype) {
    // Basic Image.
    case "image/jpg":
    case "image/jpeg":
    case "image/png":
    case "image/gif":
      if (isset($enabled_solution_packs['islandora_basic_image'])) {
        $content_model = "islandora:sp_basic_image";
      }
      break;

    // Large Image.
    case "image/tiff":
      if (isset($enabled_solution_packs['islandora_large_image'])) {
        $content_model = "islandora:sp_large_image_cmodel";
      }
      break;

    // PDF.
    case "application/pdf":
      if (isset($enabled_solution_packs['islandora_pdf'])) {
        $content_model = "islandora:sp_pdf";
      }
      break;

    // Video.
    case "video/mp4":
    case "video/quicktime":
    case "video/m4v":
    case "video/x-matroska":
    case "video/ogg":
      if (isset($enabled_solution_packs['islandora_video'])) {
        $content_model = "islandora:sp_videoCModel";
      }
      break;

    // Audio.
    case "audio/mpeg":
    case "audio/wav":
    case "audio/x-wav":
    case "audio/vnd.wave":
    case "audio/flac":
      if (isset($enabled_solution_packs['islandora_audio'])) {
        $content_model = "islandora:sp-audioCModel";
      }
      break;
  }

  return $content_model;
}
