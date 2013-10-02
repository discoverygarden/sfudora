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
 * Parse the url and teh query.
 * 
 * @param string $url
 *   The url that should be parsed.
 * 
 * @return bool|array
 *   Retrun the parsed url in an array or false.
 */
function sfudora_ajaxplorer_ingest_parse_url($url) {
  $parsed_url = FALSE;
  $parsed_url = parse_url($url);
  $query = isset($parsed_url['query']) ? $parsed_url['query'] : '';
  $query = explode('&', $query);
  $query_array = array();
  foreach ($query as $item) {
    $temp = explode('=', $item);
    $key = $temp[0];
    $value = $temp[1];
    $query_array[$key] = $value;
  }
  $parsed_url['query'] = $query_array;
  return $parsed_url;
}

/**
 * Add relationship between ajaxplorer file and islandora object.
 * 
 * @param string $download_url
 *   The url in ajaxplorer.
 * @param FedoraObject $object
 *   The object that should add relationship
 */
function sfudora_ajaxplorer_add_file_object_relationship($download_url, $object) {
  $parsed_url = sfudora_ajaxplorer_ingest_parse_url($download_url);
  $object->relationships->registerNamespace("ajxp", SFUDORA_AJXP_URI);
  if (isset($parsed_url['query']['file'])) {
    $object->relationships->add(SFUDORA_AJXP_URI, "file", $parsed_url['query']['file']);
  }

  if (isset($parsed_url['query']['repository'])) {
    $object->relationships->add(SFUDORA_AJXP_URI, "repository", $parsed_url['query']['repository']);
  }
}

/**
 * Get the object related to the ajaxplorer file.
 * 
 * @param string $search_root
 *   The root object pid.
 */
function sfudora_ajaxplorer_get_object($search_root) {
  $query = drupal_get_query_parameters();
  $repo = isset($query['repository']) ? $query['repository'] : FALSE;
  $file = isset($query['file']) ? $query['file'] : FALSE;
  $return_object = array();
  sfudora_ajaxplorer_search_object($search_root, $return_object, $repo, $file);
  print json_encode($return_object);
}

/**
 * Recuresively search object that related to the ajaxplorer file.
 * 
 * @param string $search_root
 *   The root object pid.
 * @param array $return_array
 *   The array which contains resualt object.
 * @param string $repo
 *   The repository id in ajaxplorer which passed in http query.
 * @param string $file
 *   The file path in ajaxplorer which passed in http query.
 */
function sfudora_ajaxplorer_search_object($search_root, &$return_array, $repo = FALSE, $file = FALSE) {
  $object = islandora_object_load($search_root);
  $relationship_file = $object->relationships->get(SFUDORA_AJXP_URI, 'file');
  $relationship_repo = $object->relationships->get(SFUDORA_AJXP_URI, 'repository');
  $rfile = FALSE;
  $rrepo = FALSE;
  if (!empty($relationship_file)) {
    $rfile = urldecode($relationship_file[0]['object']['value']);
  }

  if (!empty($relationship_repo)) {
    $rrepo = $relationship_repo[0]['object']['value'];
  }
  $equal = TRUE;
  if ($rrepo && $rrepo !== $repo) {
    $equal = FALSE;
  }
  if (!$file || $rfile !== $file) {
    $equal = FALSE;
  }
  if ($equal) {
    $return_array[] = array(
      'id' => $object->id,
      'label' => $object->label,
      'createdDate' => $object->createdDate,
      'state' => $object->state,
      'lastModifiedDate' => $object->lastModifiedDate,
      'owner' => $object->owner,
    );
  }
  $is_collection = FALSE;
  foreach ($object->models as $model) {
    if ($model == "islandora:collectionCModel") {
      $is_collection = TRUE;
    }
  }

  if ($is_collection) {
    $query = "SELECT DISTINCT ?object ?title
     FROM <#ri>
     WHERE {
    ?object ?collection_predicate <info:fedora/$search_root> ;
          <fedora-model:label> ?title ;
          <fedora-model:state> <fedora-model:Active> .
     FILTER(sameTerm(?collection_predicate, <fedora-rels-ext:isMemberOfCollection>))
  }ORDER BY ?title";
    $result = $object->repository->ri->query($query, 'sparql');
    foreach ($result as $obj) {
      $pid = $obj['object']['value'];
      sfudora_ajaxplorer_search_object($pid, $return_array, $repo, $file);
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
  sfudora_ajaxplorer_add_file_object_relationship($form_state['storage']['download_url'], $new_object);
  drupal_goto('islandora/object/islandora:root');
}

/**
 * Function to create ddi metadata from form.
 */
function sfudora_ajaxplorer_ingest_create_metadata(&$form, &$form_state) {

  return '';
}
