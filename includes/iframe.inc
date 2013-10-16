<?php

/**
 * @file
 * Iframe page callback (and associated, if there comes to be any?).
 */

/**
 * Page callback; render iframe containing AjaXplorer.
 */
function sfudora_ajaxplorer_frame() {
  $output = array();
  $module_path = drupal_get_path('module', 'sfudora');
  drupal_add_js("$module_path/js/ajaxplorer_frame.js");
  drupal_add_css("$module_path/css/ajaxplorer_frame.css");
  $server_base_url = $_SERVER['HTTP_HOST'];
  $sfu_path = "http://" . $server_base_url . "/" . variable_get('ajxp_path', 'ajaxplorer');
  $output['Simon Fraser University AjaXplorer'] = array(
    '#markup' => "<iframe id=\"ajaxplorer_frame\" src=\"$sfu_path\"></iframe>",
  );
  return $output;
}
