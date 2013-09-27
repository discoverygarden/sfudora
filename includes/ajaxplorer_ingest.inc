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

  // TODO: Get our DDI XML form, dump it into $form.
  $form['dummy'] = array(
    '#type' => 'item',
    '#title' => t('Dummy "field"'),
    '#markup' => t('Blahblahblah'),
  );

  $form['restrict_options'] = array(
    '#type' => 'radios',
    '#title' => t("Is this publication restricted?"),
    '#options' => array(
      'not_restricted' => t("no restriction"),
      'embargo' => t('use embargo'),
      'xacml' => t('use xacml'),
    ),
  );

  $form['embargo'] = array(
    '#type' => 'item',
    '#states' => array(
      'visible' => array(
        ':input[name=restrict_options]' => array('value' => 'embargo'),
      ),
    ),
  );

  $form['embargo']['embargo_duration'] = array(
    '#type' => 'radios',
    '#title' => t('Duration of Embargo'),
    '#options' => array(
      'indefinite' => t('Embargo indefinitely (must be lifted manually)'),
      'finite' => t('Embargo until date specified'),
    ),
    '#default_value' => 'indefinite',
  );
  $form['embargo']['embargo_date'] = array(
    '#type' => 'datepicker',
    '#title' => 'Embargo date',
    '#description' => 'The date until which this item should be embargoed (we will check with you on this date to confirm that it is ready for release)',
    '#states' => array(
      'visible' => array(
        ':input[name=embargo_duration]' => array('value' => 'finite'),
        ':input[name=restrict_options]' => array('value' => 'embargo'),
      ),
    ),
    '#default_value' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Ingest'),
  );
  return $form;
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
  // add embargo to this object
  // $objects = array("islandora:3");
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
    sfudora_ajaxplorer_ingest_set_embargo($objects, $embargo_date);
  }
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
