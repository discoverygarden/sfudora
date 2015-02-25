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
  if (!is_array($params['download_query_params'])) {
    $qp = array();
    parse_str($params['download_query_params'], $qp);
    $params['download_query_params'] = $qp;
  }
  // Check to see if a folder is tring to update.
  if ($params['is_dir'] == 'true') {
    $file_path = $params['download_query_params']['file'] . '/';
  }
  else {
    $file_path = $params['download_query_params']['file'];
  }

  $pids = sfudora_ajaxplorer_object_check($file_path, $params['repo_id']);
  if ($pids && empty($params['pid_list'])) {
    if ($params['is_dir'] == 'true') {
      drupal_set_message(t("Folders cannot be updated.  This will create a new collection."), "error");
      return drupal_get_form('sfudora_ajaxplorer_ingest_form', $params);
    }
    $params['pid_list'] = $pids;
    return drupal_get_form('sfudora_ajaxplorer_update_form', $params);
  }
  else {
    return drupal_get_form('sfudora_ajaxplorer_ingest_form', $params);
  }
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

  $form_name = "SFU DDI form";
  $form = xml_form_builder_get_form($form, $form_state, $form_name);

  $form['restrict_options'] = array(
    '#type' => 'radios',
    '#title' => t("Is this publication restricted?"),
    '#default_value' => 'not_restricted',
    '#options' => array(
      'not_restricted' => t("No restriction"),
      'embargo' => t('Use embargo'),
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
    '#value' => t('Ingest'),
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
    drupal_set_message(t("Download Url not exist"), "error");
  }

  // Set up a batch operation.
  $batch = array(
    'operations' => array(
      array('sfudora_ajaxplorer_download_file_data', array(
          $params, $download_url)),
      array('sfudora_ajaxplorer_ingest_object_batch',
        array($form, $form_state)),
    ),
    'title' => t('SFU Ingest'),
    'init_message' => t('Ingest from DataSync'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaning @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'sfudora') . '/includes/ajaxplorer_ingest.inc',
    'finished' => 'sfudora_ajaxplorer_ingest_object_finished',
  );
  batch_set($batch);
  batch_process("sfudora/ajaxplorer");
}

/**
 * Form update.
 */
function sfudora_ajaxplorer_update_form($form, &$form_state, $params) {
  form_load_include($form_state, 'inc', 'sfudora', 'includes/ajaxplorer_ingest');

  drupal_set_title("SFU Update");

  $download_url = url($params['download_base_url'], array(
    'query' => $params['download_query_params'],
    'absolute' => TRUE,
  ));

  $form_state['storage']['download_url'] = $download_url;
  $form_state['storage']['ajaxplorer_params'] = $params;

  $header = array(
    'pid' => t('Pid'),
    'label' => t('Label'),
  );

  foreach ($params['pid_list'] as $object) {
    $rows[] = array(
      'pid' => $object['pid']['value'],
      'label' => l($object['label']['value'], 'islandora/object/' . $object['pid']['value']),
    );
  }
  $form['current_object'] = array(
    '#type' => 'fieldset',
    '#title' => t('Research data exists in the repository with the same file path and ID number. Do you want to update an existing deposit?'),
    '#description' => t("Select data to update.  Leave blank to ingest new data."),
  );

  $form['current_object']['pid_table'] = array(
    '#header' => $header,
    '#type' => 'tableselect',
    '#options' => $rows,
    '#empty' => t('No results found.'),
  );

  // Set a specific callback for the submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  return $form;
}

/**
 * Form submission function.
 */
function sfudora_ajaxplorer_update_form_submit(&$form, &$form_state) {

  $update_list = array();

  foreach ($form_state['values']['pid_table'] as $checkbox => $value) {
    if ($value !== 0) {
      $update_list[] = ($form['current_object']['pid_table']['#options'][$checkbox]);
    }
  }

  // If the list is empty then go to ingest form.
  if (!$update_list) {
    $url = url("sfudora/ingest/", array('absolute' => TRUE, 'query' => $form_state['storage']['ajaxplorer_params']));
    drupal_goto($url);
  }
  // Get the ajaxplorer params.
  $params = $form_state['storage']['ajaxplorer_params'];
  $download_url = "";
  if (isset($form_state['storage']['download_url'])) {
    $download_url = $form_state['storage']['download_url'];
  }
  else {
    drupal_set_message(t("Download Url not exist"), "error");
  }

  // Set up a batch operation.
  $batch = array(
    'operations' => array(
      array('sfudora_ajaxplorer_download_file_data', array(
        $params, $download_url)),
      array('sfudora_ajaxplorer_update_object_batch',
        array($form, $form_state, $update_list)),
    ),
    'title' => t('SFU Update'),
    'init_message' => t('Update from DataSync'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaning @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'sfudora') . '/includes/ajaxplorer_ingest.inc',
    'finished' => 'sfudora_ajaxplorer_update_object_finished',
  );
  batch_set($batch);
  batch_process("sfudora/ajaxplorer");
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
    $root_collection = variable_get('sfudora_repository_pid', 'islandora:root');
    drupal_goto('islandora');
  }
}

/**
 * The batch process of ingest object.
 */
function sfudora_ajaxplorer_update_object_batch($form, $form_state, $update_list, &$context) {
  $file = $context['results']['files'][0];
  $result = &$context['results'];
  $sandbox = &$context['sandbox'];
  if (!isset($sandbox['offset'])) {
    $sandbox['offset'] = 0;
    $sandbox['total'] = count($update_list);
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

  sfudora_ajaxplorer_update_obj_datastream($form, $form_state, $file, $update_list[$offset]);
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
function sfudora_ajaxplorer_update_object_finished($success, $results, $operations) {
  if ($success) {
    $root_collection = variable_get('sfudora_repository_pid', 'islandora:root');
    drupal_goto('islandora');
  }
}

/**
 * Ingest a single object into fedora.
 *
 * @param array $form
 *   The form.
 * @param string $form_state
 *   The form state.
 * @param string $file_info
 *   Information on the file. ex file paths, file name, parent collection etc.
 *
 * @return string
 *   The new object pid.
 */
function sfudora_ajaxplorer_ingest_single_object(&$form, &$form_state, $file_info, $result) {
  module_load_include('inc', 'islandora', 'includes/utilities');

  $parent_collection = variable_get('sfudora_repository_pid', 'islandora:root');
  // If the parent collection is not sfudora root then find the pid of the
  // parent collection.
  if (isset($file_info->parent_collection) && $file_info->parent_collection != $parent_collection) {
    $parent_collection = $result['islandora_batch_ingest_object'][$file_info->parent_collection];
  }

  // Depending if the the batch is a single file or folder, the title will be
  // either the form title or the file name.
  $title = isset($file_info->file_name) ? $file_info->file_name : $form_state['values']['stdyDscr']['citation']['titlStmt']['titl'];

  // Set the file name to be the file info file name or the ajaxplorer
  // file path.
  $file_info->file_name = isset($file_info->file_name) ? $file_info->file_name : $file_info->ajaxplorer_file_path;

  $content_model = isset($file_info->content_model) ? $file_info->content_model : sfudora_ajaxplorer_get_content_model($file_info->file_name);
  $new_object = islandora_prepare_new_object("sfu", $title, array(), array($content_model), array(array('relationship' => 'isMemberOfCollection', 'pid' => $parent_collection)));

  // If the object is a file then add the OBJ and DDI datastream.
  if (isset($file_info->is_file) && $file_info->is_file == TRUE) {
    // Create OBJ datastream.
    sfudora_ajaxplorer_ingest_create_obj_datastream($form, $form_state, $new_object, $file_info);
    // Create the DDI datastream.
    sfudora_ajaxplorer_ingest_create_ddi_datastream($form, $form_state, $new_object);
  }

  if ($content_model == 'islandora:collectionCModel') {
    sfudora_ajaxplorer_ingest_create_policy_datastream($new_object, $parent_collection);
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

  // Set the xacml policy datastream.
  if (isset($form_state['values']['restrict_options']) &&
      $form_state['values']['restrict_options'] == 'xacml') {
    sfudora_ajaxplorer_ingest_set_xacml($new_object, $form_state['values']['xacml']);
  }

  // Add the ajaxplorer file path and repository id to RELS-EXT
  $repo_id = $form_state['storage']['ajaxplorer_params']['repo_id'];
  sfudora_ajaxplorer_add_file_object_relationship($file_info->ajaxplorer_file_path, $repo_id, $new_object);

  // Add object.
  islandora_add_object($new_object);
  // Return the new object pid.
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
 * Creates a COLLECTION_POLICY Datastream.
 *
 * @param object $new_object
 *   The new Object.
 * @param string $parent_collection 
 *   The parent collection
 */
function sfudora_ajaxplorer_ingest_create_policy_datastream($new_object, $parent_collection) {
  $collection = islandora_object_load($parent_collection);
  $policy_xml = $collection['COLLECTION_POLICY']->content;
  $policy = new CollectionPolicy($policy_xml);
  // Create the OBJ datastream.
  $obj_ds = $new_object->constructDatastream('COLLECTION_POLICY', 'M');
  $obj_ds->label = 'COLLECTION_POLICY';
  $obj_ds->mimeType = 'text/xml';
  $obj_ds->setContentFromString($policy->getXML());
  $new_object->ingestDatastream($obj_ds);
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
 * Updates a OBJ Datastream.
 *
 * @param array $form
 *   The form.
 * @param string $form_state
 *   The form state.
 * @param object $file_info
 *   The file info.
 * @param object $update_object
 *   The pid of the object to update.
 *
 * @return string
 *   Returns the temporary file path of the downloaded file.
 */
function sfudora_ajaxplorer_update_obj_datastream(&$form, &$form_state, $file_info, $update_object) {

  $pid = $update_object['pid'];
  $object = islandora_object_load($pid);
  $object['OBJ']->setContentFromFile($file_info->uri, FALSE);
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
 * @param string|AbstractObject $object
 *   The object (or its pid) that the xacml should be added to
 * @param string $xacml
 *   The real path pf the xacml file.
 */
function sfudora_ajaxplorer_ingest_set_xacml($object, $xacml) {

  $xacml_ds = $object->constructDatastream('POLICY', 'X');
  $xacml_ds->setContentFromFile($xacml);
  $xacml_ds->label = "XACML Policy Stream";
  if ($object->getDatastream('POLICY')) {
    $object->purgeDatastream('POLICY');
  }

  $object->ingestDatastream($xacml_ds);
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
  $download_url_path_info = pathinfo($download_url);
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
    // Creates file and collection arrays, storing in the context results.
    sfudora_ajaxplorer_get_folder_data($temp_file_path, $context);
  }
  else {
    sfudora_ajaxplorer_get_file_data($params, $temp_file_path, $context);
  }
}

/**
 * Get the folder information.
 *
 * @param string $file_data
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
    $file_entry->parent_collection = '/' . $path_info['dirname'] . '/';
    $file_entry->ajaxplorer_file_path = '/' . $name;
    $file_entry->file_name = $path_info['basename'];

    if (isset($path_info['extension']) && strpos($path_info['dirname'], '__MACOSX') === FALSE && $path_info['extension'] != 'DS_Store') {
      $file_entry->uri = "zip://$zip_file#$name";
      $files_info[] = $file_entry;
      $file_entry->is_file = TRUE;
    }
    else {
      $file_entry->parent_collection = $file_entry->parent_collection == "/./" ? $root_collection : $file_entry->parent_collection;
      $file_entry->content_model = "islandora:collectionCModel";
      $collections_info[] = $file_entry;
    }
  }

  $files_info = array_merge($collections_info, $files_info);

  $zip->close();

  // Store the informatino into th context.
  $context['results']['files'] = $files_info;
  $context['results']['temp_zip'] = $zip_file;
}

/**
 * Get the folder information.
 *
 * @param array $params
 *   The ajaxplorer parameters.
 * @param string $file_path
 *   The download url of file.
 */
function sfudora_ajaxplorer_get_file_data($params, $file_path, &$context) {

  $file = drupal_realpath($file_path);

  $root_collection = variable_get('sfudora_repository_pid', 'islandora:root');

  $files_info = array();
  $file_entry = new stdClass();
  $file_entry->parent_collection = $root_collection;
  $file_entry->ajaxplorer_file_path = $params['download_query_params']['file'];
  $file_entry->is_file = TRUE;
  $file_entry->uri = $file;
  $files_info[] = $file_entry;

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

  $ajxp_namespace = SFUDORA_AJXP_URI;
  $query =
    "PREFIX sfudora:<$ajxp_namespace>
    SELECT ?pid ?label
    FROM <#ri>
    WHERE
    {
      ?pid sfudora:file \"$file_path\" ;
      sfudora:repository \"$repo_id\" ;
      <fedora-model:label> ?label
    }";
  
  $result = $repo->repository->ri->sparqlQuery($query);
  return $result;
  
  
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
  $content_model = "islandora:binaryObjectCModel";

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
    case "video/x-msvideo";
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

    // Tabular Data.
    case "text/csv":
    case "application/x-spss-sav":
    case "application/vnd.ms-excel":
    case "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet":
      if (isset($enabled_solution_packs['islandora_data'])) {
        $content_model = "islandora:sp_data";
      }
      break;
  }

  return $content_model;
}
