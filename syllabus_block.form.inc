<?php
/**
 * @file
 * syllabus_block.form.inc
 *
 * @author Luc Morin <lmori4@uottawa.ca>
 * @copyright 2012 University of Ottawa.
 * @license Copyright (c) 2012 All rights reserved
 */


/**
 * Implements hook_form().
 */
function syllabus_block_search_result_form($form, $form_state) {
  //Check if both are set if not redirect back.
  if (!isset($_GET['course_cd']) || !isset($_GET['session_cd'])) {
    drupal_goto('dashboard');
  }
  $result_table = $rows = array();

  $course_code = filter_xss($_GET['course_cd']);
  $session = filter_xss($_GET['session_cd']);

  module_load_include('inc', 'syllabus_block', 'syllabus_block.block');
  $form = syllabus_search_form($form, $form_state);
  //Set the defaults,
  $form['course_code']['#default_value'] = $course_code;
  $form['session']['#default_value'] = $session;

  $header = array(
    array('data' => t('Course code'), 'field' => 'fcc.field_course_code_value'),
    array('data' => t('Section'), 'field' => 'fsec.field_section_value'),
    array('data' => t('Session'), 'field' => 'fs.field_session_value'),
  ); 

  //db_select to make tables sortable to find the list of courses
  $db_query = db_select('node', 'n')->extend('TableSort');
  $db_query->fields('n');
  $db_query->join('field_data_field_public', 'fp', 'n.nid = fp.entity_id');
  $db_query->join('field_data_field_session', 'fs', 'n.nid = fs.entity_id');
  $db_query->join('field_data_field_course_code', 'fcc', 'n.nid = fcc.entity_id');
  $db_query->join('field_data_field_section', 'fsec', 'n.nid = fsec.entity_id');
  $db_query->condition('n.type', 'syllabus' ,'=');
  $db_query->condition('n.status', '1' ,'=');
  $db_query->condition('fp.field_public_value', '1' ,'=');
  
  //If the course code is set
  if (!empty($course_code)) {
    $db_query->condition('n.title', $course_code . '%' ,'like');
  }
  //If the session is set.
  if (!empty($session)) {
    $db_query->condition('fs.field_session_value', $session, '=');
  }
  $db_result = $db_query->orderByHeader($header)->execute();

  foreach ($db_result as $result_node) {
    $node = node_load($result_node->nid);
    //Create the rows.
    $rows[] = array(array('data' => l($node->field_course_code[LANGUAGE_NONE][0]['value'], 'syllabus/' . $node->nid), 'align' => 'left'),
                    array('data' => $node->field_section[LANGUAGE_NONE][0]['value'], 'align' => 'left'),
                    array('data' => $node->field_session[LANGUAGE_NONE][0]['value'], 'align' => 'left'));
  }
  $table_attributes = array('id' => 'search-result-table');
  $variables['header'] = $header;
  $variables['rows'] = $rows;
  $variables['attributes'] = $table_attributes;
  $variables['caption'] = '';    
  $variables['colgroups'] = array();
  $variables['sticky'] = '';
  $variables['empty'] = t('No results found.');
  $form['table'] = array(
    '#markup' => theme_table($variables),
  );

  return $form;
}
