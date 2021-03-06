<?php

/**
 * @file
 * Plugin definition and callbacks for a ctools:content_types plugin.
 *
 * @ingroup CToolsPlugin CToolsPluginContentTypes
 */

$plugin = array(
  'single' => TRUE,
  'title' => t('Travel Agency: Travels'),
  'description' => t('Show the travels'),
  'category' => t('Travel Agency'),
  'edit form' => 'travel_agency_travels_pane_custom_pane_edit_form',
  'render callback' => 'travel_agency_travels_pane_custom_pane_render',
  'admin info' => 'travel_agency_travels_pane_custom_pane_admin_info',
  'defaults' => array(
    'text' => 'Travels',
  ),
  'all contexts' => TRUE,
);

/**
 * An edit form for the pane's settings.
 */
function travel_agency_travels_pane_custom_pane_edit_form($form, &$form_state) {
  $conf = $form_state['conf'];

  $form['text'] = array(
    '#type' => 'textfield',
    '#title' => t('Travels text'),
    '#description' => t('Text to display, it may use substitution strings'),
    '#default_value' => $conf['text'],
  );

  return $form;
}

/**
 * Submit the customers_edit form.
 */
function travel_agency_travels_pane_custom_pane_edit_form_submit(&$form, &$form_state) {
  foreach (array_keys($form_state['plugin']['defaults']) as $key) {
    if (isset($form_state['values'][$key])) {
      $form_state['conf'][$key] = $form_state['values'][$key];
    }
  }
}

/**
 * Admin info callback.
 *
 * @param string $subtype
 *   Identifier of subtype.
 * @param array $conf
 *   Pane configuration. Array keys are depends on the _edit_form.
 *   - override_title
 *   - override_title_text.
 * @param array $context
 *   An array of context objects available for use. These may be placeholders.
 *
 * @return object
 *   Administrative title and description of the $subtype pane.
 *   The keys are:
 *   - title: string
 *   - content: string|array
 */
function travel_agency_travels_pane_custom_pane_admin_info($subtype, $conf, $context) {
  if (!empty($conf)) {
    $block = new stdClass;
    $block->title = $conf['override_title'] ? $conf['override_title_text'] : '';
    $block->content = $conf['text'];
    return $block;
  }
}

/**
 * Render callback.
 *
 * "CTools:Content types" plugin render callback for "Icf:Icf full search".
 *
 * @param string $subtype
 *   Subtype identifier.
 * @param array  $conf
 *   Configuration of the $subtype instance.
 * @param array  $args
 *   Documentation missing.
 * @param array  $context
 *   Documentation missing.
 *
 * @return object
 *   The content.
 */
function travel_agency_travels_pane_custom_pane_render($subtype, $conf, $args, $context) {
  
  $content = _ta_travels_pane_list() .
  theme('html_tag', array(
    'element' => array(
      '#tag' => 'em',
      '#attributes' => array(
        'class' => 'reference-comment',
        ),
      '#value' => 'This table generated by the ta_travels_pane.inc.',
    ),
  ));

  $block = new stdClass();

  $block->title = t('Travels');
  $block->content = $content;
 
  return $block;
   
}

/**
 * Join the "User" and the "field_data_field_telephone_number" tables.
 * 
 */
function _ta_travels_pane_list() {

  $header = array(
    array('data' => 'Travel', 'field' => 'title', 'sort' => 'ASC'),
    array('data' => 'Location', 'field' => 'city', 'sort' => 'ASC'),
    array('data' => 'Start date', 'field' => 'open', 'sort' => 'ASC'),
    array('data' => 'End date', 'field' => 'close', 'sort' => 'ASC'),
    array('data' => 'Status', 'sort' => 'ASC'),
  );
  
  $query = db_select('node', 'travels_n')
        ->extend('PagerDefault')
        ->extend('TableSort');
  
  $query->join('location_instance', 'travels_li',
        'travels_n.nid = travels_li.nid');
  
  $query->join('location', 'travels_l',
        'travels_li.lid = travels_l.lid');
  
  $query->join('registration_entity', 'travels_re',
        'travels_n.nid = travels_re.entity_id');
  
  $query->condition('type', 'travel', '=')
        ->condition('entity_type', 'node', '=')
        ->fields('travels_n', array('title'))
        ->fields('travels_l', array('city', 'country'))
        ->fields('travels_re', array('open', 'close'))
        ->limit(7)
        ->orderByHeader($header)
        ->orderBy('title', 'DESC');

  $results = $query->execute();

  $rows = array();
  foreach ($results as $row) {
    $rows[] = array($row->title,
                    $row->city .= ' (' . _ta_travels_pane_country_code_to_country_name($row->country) . ')',
                    $row->open,
                    $row->close,
                    $row->status = _ta_travels_pane_travel_status_checker($row->open, $row->close),
    );
  }

  $output = theme('table', array('header' => $header,
                                 'rows' => $rows ));
  $output .= theme('pager');

  return $output;

}

/**
* Convert ISO country code to country name.
*
* @param string $country_code
*   Country ISO code. Can be of any case.
*/
function _ta_travels_pane_country_code_to_country_name($country_code) {
  // Necessary for country_get_list().
  require_once DRUPAL_ROOT . '/includes/locale.inc';
      $countries = country_get_list();
      $cc = strtoupper($country_code);
      return isset($countries[$cc]) ? $countries[$cc] : FALSE ;
  }
  
/**
* Check the travel is open, in progress, closed or invalid 
*
* @param date $starting_date
*   The starting date of the travel.
* 
* @param date $ending_date
*   The ending date of the travel.
*
* @return string
*   If starting date > ending date return string = invalid.
*   If starting date > current date return string = open.
*   If starting date < current date and ending date > current date
*     return string = in progress.
*   If ending date < current date return string = closed.  
*/
function _ta_travels_pane_travel_status_checker($starting_date, $ending_date) {

  $status = 'no value';
  $current_date = date('Y-m-d h:m:s');
  
  if ($starting_date > $ending_date) {
    $status = 'invalid';
  }
  else {
      
    if ($starting_date > $current_date) {
      $status = 'open';
    }
    elseif ($ending_date < $current_date) {
      $status = 'closed';
    }
    elseif ($starting_date < $current_date && $ending_date > $current_date) {
      $status = 'in progress';
    }
  }
 
  return $status;
  
}