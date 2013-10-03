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
  $module_path = drupal_get_path('module', 'islandora_basic_image');

  $form_name = "Basic image MODS form";
  $xml = file_get_contents("$module_path/xml/islandora_basic_image_form_mods.xml");

  $form = xml_form_builder_get_form($form, $form_state, $form_name, $xml);

  // Set a specific callback for the submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}


/**
 * Form validation function.
 */
function sfudora_ajaxplorer_ingest_form_validate(&$form, &$form_state) {
  // TODO: Do the XML Form validation stuffs...
}

/**
 * Form submission function.
 */
function sfudora_ajaxplorer_ingest_form_submit(&$form, &$form_state) {
  // TODO: Do the XML form validation stuff to get the output XML.
  // TODO: Actually create objects... If we're a folder, download the ZIP,
  // iterate over all the contents, creating objects as necessary.
  // We need to determine if single or folder ingest.
  // We need to determine if object is generic based on file type.
  module_load_include('inc', 'islandora', 'includes/utilities');
  $content_model = 'sfu:genericCModel';
  $parent_collection = 'islandora:root';

  // Need to change this to be the metadata title.
  $title = 'test';
  $new_object = islandora_prepare_new_object("test", $title, array(), array($content_model), array(array('relationship' => 'isMemberOfCollection', 'pid' => $parent_collection)));

  $obj_label = 'Generic Record';
  $obj_mimetype = 'image/jpeg';

  // Create the MODS datastream.
  $obj_ds = $new_object->constructDatastream('OBJ', 'M');
  $obj_ds->label = $obj_label;
  $obj_ds->mimeType = $obj_mimetype;
  $obj_ds->setContentFromUrl($form_state['storage']['download_url']);
  $new_object->ingestDatastream($obj_ds);

  // Create the MODS datastream.
  $ddi_metadata = sfudora_ajaxplorer_ingest_create_metadata($form, $form_state);
  $ddi_ds = $new_object->constructDatastream('DDI', 'M');
  $ddi_ds->label = 'DDI Record';
  $ddi_ds->mimeType = 'text/xml';
  $ddi_ds->setContentFromString($ddi_metadata);
  $new_object->ingestDatastream($ddi_ds);

  // Store the object.
  islandora_add_object($new_object);
  drupal_goto('islandora/object/islandora:root');

}


/**
 * Function to create ddi metadata from form.
 */
function sfudora_ajaxplorer_ingest_create_metadata(&$form, &$form_state) {

  return '';

}
