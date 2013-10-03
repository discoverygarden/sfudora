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
    sfudora_foulder_ingest_form($form, $form_state, $params);
  }
  return $form;
}

/**
 * Hide the title of object ingest form.
 * 
 * @param array $params
 *   The parameters that ajaxplorer passed.
 */
function sfudora_foulder_ingest_form(&$form, &$form_state, $params) {
  $file = $params['download_query_params']['file'];
  $path = explode('/', $file);
  $name = end($path);
  $form['titleInfo']['title']['#type'] = 'hidden';
  $form['titleInfo']['title']['#value'] = $name;
}

/**
 * Ingest a single object to a collection.
 * 
 * @param string $parent_collection
 *   The collection that the object should be ingested into
 */
function sfudora_ingest_single_object(&$form, &$form_state, $parent_collection = 'islandora:root') {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $content_model = 'sfu:genericCModel';

  // Need to change this to be the metadata title.
  $title = $form_state['values']['titleInfo']['title'];

  // Create the new Fedora Object.
  $new_object = islandora_prepare_new_object("sfu", $title, array(), array($content_model), array(array('relationship' => 'isMemberOfCollection', 'pid' => $parent_collection)));

  // Create the OBJ datastream.
  sfudora_ajaxplorer_ingest_create_obj_datastream($form, $form_state, $new_object);
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

  // Store the Fedora Object.
  islandora_add_object($new_object);
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
  if ($params['is_dir'] == "true") {
    sfudora_ingest_folder($form, $form_state);
  }
  else {
    sfudora_ingest_single_object($form, $form_state);
  }
  drupal_goto('islandora/object/islandora:root');
}

/**
 * Ingest a ajaxplorer folder as a collection in islandora.
 */
function sfudora_ingest_folder(&$form, &$form_state) {
  $folder = sfudora_ajaxplorer_get_file_data($form_state['storage']['download_url']);
  $tmp_path = file_directory_temp();
  $filename = $tmp_path . '/' . $form_state['values']['titleInfo']['title'] . '.zip';
  file_put_contents($filename, $folder);
  $output = shell_exec("unzip $filename -d $tmp_path");
  $tmp_folder = $tmp_path . '/' . $form_state['values']['titleInfo']['title'];
  sfudora_ingest_collection($form, $form_state, $tmp_folder, "islandora:root");
  file_unmanaged_delete($filename);
  file_unmanaged_delete_recursive($tmp_folder);
}

/**
 * Recursively ingest the collection.
 * 
 * @param string $filedir
 *   The directory of the folder.
 * @param string $parent
 *   The parent folder of the current folder.
 */
function sfudora_ingest_collection(&$form, &$form_state, $filedir, $parent) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  $content_model = 'islandora:collectionCModel';
  $parent_collection = $parent;
  $title = $form_state['values']['titleInfo']['title'];
  // Create the new Fedora Object.
  $new_object = islandora_prepare_new_object("sfu", $title, array(), array($content_model), array(array('relationship' => 'isMemberOfCollection', 'pid' => $parent_collection)));
  // Store the Fedora Object.
  islandora_add_object($new_object);
  $files = scandir($filedir);
  foreach ($files as $key => $file) {
    if (($file !== '.') && ($file !== '..')) {
      $name_array = explode('.', $file);
      $name = "";
      $is_dir = is_dir($filedir . '/' . $file);
      for ($i = 0; $i < count($name_array) - 1; $i++) {
        $name = $name . $name_array[$i];
      }
      if ($is_dir) {
        $name = $name . end($name_array);
      }
      $form['titleInfo']['title']['#value'] = $name;
      $form_state['values']['titleInfo']['title'] = $name;
      if ($is_dir) {
        sfudora_ingest_collection($form, $form_state, $filedir . '/' . $file, $new_object->id);
      }
      else {
        $form_state['storage']['url'] = $filedir . '/' . $file;
        unset($form_state['storage']['download_url']);
        sfudora_ingest_single_object($form, $form_state, $new_object->id);
      }
    }
  }
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
function sfudora_ajaxplorer_ingest_create_obj_datastream(&$form, &$form_state, $new_object) {
  module_load_include('inc', 'islandora', 'includes/mime_detect');

  $obj_label = 'Generic Record';

  $mime_detect = new MimeDetect();
  $obj_mimetype = $mime_detect->getMimetype($form_state['storage']['ajaxplorer_params']['download_query_params']['file']);

  // Create the OBJ datastream.
  $obj_ds = $new_object->constructDatastream('OBJ', 'M');
  $obj_ds->label = $obj_label;
  $obj_ds->mimeType = $obj_mimetype;
  $file_content = "";
  if (isset($form_state['storage']['download_url'])) {
    $file_content = sfudora_ajaxplorer_get_file_data($form_state['storage']['download_url']);
  }
  elseif (isset($form_state['storage']['url'])) {
    $file_content = file_get_contents($form_state['storage']['url']);
  }
  $obj_ds->setContentFromString($file_content);
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

  $ch = curl_init($download_url);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  curl_setopt($ch, CURLOPT_COOKIE, $cookies);
  // Store the response into the variable $file_data.
  $file_data = curl_exec($ch);
  curl_close($ch);

  return $file_data;
}
