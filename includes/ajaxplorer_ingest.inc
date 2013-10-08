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
 * files.
 * - download_base_url: Where to hit Ajaxplorer.
 * - download_query_params: URL-encoded parameters to throw back at Ajaxplorer
 * when we go to download whatever resource. We'll parse 'em into an
 * associative array so they're easier to use later.
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
    to_show = jQuery('#block-system-main, #block-system-main *').parentsUntil('body').andSelf();
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
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Ingest'),
  );

  if ($params['is_dir'] == 'true') {
    sfudora_ajaxplorer_foulder_ingest_form($form, $form_state, $params);
  }
  return $form;
}

/**
 * Hide the title of object ingest form.
 * 
 * @param array $params
 *   The parameters that ajaxplorer passed.
 */
function sfudora_ajaxplorer_foulder_ingest_form(&$form, &$form_state, $params) {
  $file = $params['download_query_params']['file'];
  $path = explode('/', $file);
  $name = end($path);
  $form['titleInfo']['title']['#type'] = 'hidden';
  $form['titleInfo']['title']['#value'] = $name;
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
function sfudora_ajaxplorer_ingest_single_object(&$form, &$form_state, $parent_collection = 'islandora:root') {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $content_model = 'sfu:genericCModel';

  // Need to change this to be the metadata title.
  $title = $form_state['values']['titleInfo']['title'];
  if (isset($form_state['storage']['ajaxplorer_params']['is_dir']) &&
      $form_state['storage']['ajaxplorer_params']['is_dir'] == "true") {
    $content_model = 'islandora:collectionCModel';
  }
  // Create the new Fedora Object.
  $new_object = islandora_prepare_new_object("sfu", $title, array(), array($content_model), array(array('relationship' => 'isMemberOfCollection', 'pid' => $parent_collection)));

  // Create the OBJ datastream.
  $file_path = "";
  if (isset($form_state['storage']['download_url'])) {
    $file_data = sfudora_ajaxplorer_get_file_data($form_state['storage']['download_url']);
    $file_path = drupal_realpath($file_data['path']);
  }
  else {
    $file_path = $form_state['storage']['url'];
  }
  sfudora_ajaxplorer_ingest_create_obj_datastream($form, $form_state, $new_object, $file_path);
  // Create the DDI datastream.
  sfudora_ajaxplorer_ingest_create_ddi_datastream($form, $form_state, $new_object);

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

  $param = $form_state['storage']['ajaxplorer_params'];
  $ajax_url = "";
  if (isset($form_state['storage']['download_url'])) {
    $ajax_url = $param['download_query_params'];
  }
  else {
    $file_path = explode('/', $param['download_query_params']['file']);
    $root_path = "";
    for ($i = 0; $i < count($file_path) - 1; $i++) {
      $root_path .= $file_path[$i] . '/';
    }
    $param['download_query_params']['file'] = str_replace(file_directory_temp() . "/", $root_path, $form_state['storage']['url']);
    $ajax_url = $param['download_query_params'];
  }
  sfudora_ajaxplorer_add_file_object_relationship($ajax_url, $new_object);
  islandora_add_object($new_object);
  return $new_object->id;
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
  // TODO: Do the XML form validation stuff to get the output XML.
  // TODO: Actually create objects... If we're a folder, download the ZIP,
  // iterate over all the contents, creating objects as necessary.
  // TODO: We need to determine if single or folder ingest.
  // TODO: We need to determine if object is generic based on file type.
  $params = $form_state['storage']['ajaxplorer_params'];
  $download_url = "";
  if (isset($form_state['storage']['download_url'])) {
    $download_url = $form_state['storage']['download_url'];
  }
  else {
    drupal_set_message(t("Download Url not exist"), "Error");
  }
  if (isset($params['is_dir']) && $params['is_dir'] == "true") {
    $batch = array(
      'operations' => array(
        array('sfudora_ajaxplorer_get_folder', array($download_url)),
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
  else {
    sfudora_ajaxplorer_ingest_single_object($form, $form_state);
  }
}

/**
 * Get the folder information.
 * 
 * @param string $download_url
 *   The download url in ajaxplorer.
 */
function sfudora_ajaxplorer_get_folder($download_url, &$context) {
  $context['message'] = t('Downloading folders.');
  $file_data = sfudora_ajaxplorer_get_file_data($download_url);
  $zip_file = drupal_realpath($file_data['path']);
  $zip = new ZipArchive();
  $files = array();
  if ($zip->open($zip_file)) {
    $zip->extractTo(file_directory_temp());
    $zip->close();
  }

  $zip = zip_open($zip_file);
  if ($zip) {
    while ($file = zip_read($zip)) {
      $files[] = zip_entry_name($file);
    }
    zip_close($zip);
  }

  $context['results']['sfudora_folder_files'] = $files;
  $context['results']['temp_zip'] = $zip_file;
  $context['results']['temp_folder'] = file_directory_temp() . "/" . $files[0];
}

/**
 * The batch process of ingest object.
 */
function sfudora_ajaxplorer_ingest_object_batch($form, $form_state, &$context) {
  $files = $context['results']['sfudora_folder_files'];
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
  $entry_name = $files[$offset];
  $file_name = explode("/", $entry_name);
  $name_num = count($file_name) - 1;
  if (!$file_name[$name_num]) {
    unset($file_name[$name_num]);
    $form_state['storage']['ajaxplorer_params']['is_dir'] = "true";
  }
  $collection = "/";
  for ($i = 0; $i < count($file_name) - 1; $i++) {
    $collection .= $file_name[$i] . "/";
  }
  if (isset($result['islandora_batch_ingest_object'][$collection])) {
    $collection = $result['islandora_batch_ingest_object'][$collection];
  }
  else {
    $collection = "islandora:root";
  }
  $name = end($file_name);
  $form['titleInfo']['title']['#value'] = $name;
  $form_state['values']['titleInfo']['title'] = $name;
  unset($form_state['storage']['download_url']);
  $form_state['storage']['url'] = file_directory_temp() . "/" . $entry_name;
  $pid = sfudora_ajaxplorer_ingest_single_object($form, $form_state, $collection);
  $result['islandora_batch_ingest_object']["/" . $entry_name] = $pid;
  $sandbox['offset']++;
  if ($sandbox['offset'] >= $sandbox['total']) {
    file_unmanaged_delete($context['results']['temp_zip']);
    file_unmanaged_delete_recursive($context['results']['temp_folder']);
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
 * Get folder data using curl.
 */
function sfudora_ajaxplorer_ingest_folder_get_data(&$form, &$form_state) {
  $folder = sfudora_ajaxplorer_get_file_data($form_state['storage']['download_url'], TRUE);
  $zip = new ZipArchive();
  $zip_file = drupal_realpath($folder);
  if (($error = $zip->open($zip_file)) !== TRUE) {
    drupal_set_message(t("error occured when unzip the file"), "error");
  }
  $tmp_folder = file_directory_temp() . '/' . $form_state['values']['titleInfo']['title'];
  $zip->extractTo(file_directory_temp());
  $zip->close();
  return array('tmp_folder' => $tmp_folder, 'zip_file' => $zip_file);
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
 */
function sfudora_ajaxplorer_ingest_create_obj_datastream(&$form, &$form_state, $new_object, $file_path) {
  module_load_include('inc', 'islandora', 'includes/mime_detect');

  $obj_label = 'Generic Record';

  $mime_detect = new MimeDetect();
  $obj_mimetype = $mime_detect->getMimetype($form_state['storage']['ajaxplorer_params']['download_query_params']['file']);

  // Create the OBJ datastream.
  $obj_ds = $new_object->constructDatastream('OBJ', 'M');
  $obj_ds->label = $obj_label;
  $obj_ds->mimeType = $obj_mimetype;
  $obj_ds->setContentFromFile($file_path);
  $new_object->ingestDatastream($obj_ds);
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
 * @returns string $file_data
 *   The file data as a string.
 */
function sfudora_ajaxplorer_get_file_data($download_url) {

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
  // Store the response into the variable $file_data.
  curl_exec($ch);
  $info = curl_getinfo($ch);
  curl_close($ch);
  fclose($fp);

  return array("path" => $temp_file_path, "info" => $info);
}

/**
 * Get an array of object which is related to the ajaxplorer file.
 * 
 * @param string $repo
 *   The ajaxplorer id to search.
 * @param string $file
 *   The ajaxplorer file path to search.
 * @param string $search_root
 *   The root file of the repository.
 * 
 * @return array
 *   An array of result pid.
 */
function sfudora_ajaxplorer_get_object($repo = FALSE, $file = FALSE, $search_root = 'islandora:root') {
  $object = islandora_object_load($search_root);
  if (!($repo === FALSE) && $file) {
    $query = "SELECT ?object from <#ri>
      WHERE {?object <http://ajaxplorer.info#file> \"" . $file . "\".
       ?object <http://ajaxplorer.info#repository> \"" . $repo . "\".
  }";
  }
  elseif ($file) {
    $query = "SELECT ?object from <#ri>
      WHERE {?object <http://ajaxplorer.info#file> \"" . $file . "\".
  }";
  }
  $result = $object->repository->ri->query($query, 'sparql');
  $pids = array();
  foreach ($result as $array) {
    if (isset($array['object'])) {
      $object = $array['object'];
      if ($object['type'] == 'pid') {
        $pids[] = $object['value'];
      }
    }
  }
  return $pids;
}

/**
 * Add relationship between ajaxplorer file and islandora object.
 *
 * @param string $parsed_url
 *   The url in ajaxplorer.
 * @param FedoraObject $object
 *   The object that should add relationship
 */
function sfudora_ajaxplorer_add_file_object_relationship($parsed_url, $object) {
  $object->relationships->registerNamespace("ajxp", SFUDORA_AJXP_URI);
  if (isset($parsed_url['file'])) {
    $object->relationships->add(SFUDORA_AJXP_URI, "file", $parsed_url['file'], TRUE);
  }

  if (isset($parsed_url['repository'])) {
    $object->relationships->add(SFUDORA_AJXP_URI, "repository", $parsed_url['repository'], TRUE);
  }
}
