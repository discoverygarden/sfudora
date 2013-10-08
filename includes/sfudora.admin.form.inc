<?php

/**
 * @file
 * Form handling for the administration of Simon Fraser University.
 */

/**
 * Defines the Simon Fraser University admin form.
 *
 * @param array $form
 *   An array representing a Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   An array representing the Simon Fraser University admin form.
 */
function sfudora_admin_form($form, &$form_state) {

  $form = array();

  $form['root_collection'] = array(
    '#type' => 'textfield',
    '#title' => t('Root Collection PID for ingest'),
    '#default_value' => variable_get('sfudora_repository_pid', 'islandora:root'),
    '#description' => t('The PID of the Root Collection Object'),
    '#required' => TRUE,
  );

  // Set a specific callback for the submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Configuration'),
  );

  return $form;
}

/**
 * Form validation function.
 */
function sfudora_admin_form_validate(&$form, &$form_state) {

  $object = islandora_object_load($form_state['values']['root_collection']);
  if (!$object) {
    form_set_error('root_collection', t($form_state['values']['root_collection'] . ' does not exist. Enter a valid PID.'));
  }
}

/**
 * Form submission function.
 */
function sfudora_admin_form_submit(&$form, &$form_state) {

  variable_set('sfudora_repository_pid', $form_state['values']['root_collection']);
  drupal_set_message(t('Simon Fraser University settings saved.'), 'status');
}
