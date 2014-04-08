<?php
/**
 * @file
 * syllabus_block.block.inc
 *
 * @author Luc Morin <lmori4@uottawa.ca>
 * @copyright 2012 University of Ottawa.
 * @license Copyright (c) 2012 All rights reserved
 */

/**
 * Implements hook_form().
 */
function syllabus_pending_form($form, &$form_state) {
  global $language; 
  module_load_include('inc', 'syllabus_api', 'syllabus_api.session');
  $groups = og_get_groups_by_user();
  //List the syllabuses that the prof has still not published a syllabus for.
  $form['pending_text'] = array(
    '#markup' => t('Here is a list of the pending syllabus.'),
  );
  $entities = entity_load('academic_activity', $groups['academic_activity']);
  //Fetch the sessions 
  $sessions = syllabus_api_fetch_sessions();

  $form['pending_list'] = array(
    '#prefix' => '<ul>',
    '#markup' => '',
    '#suffix' => '</ul>',
  );

  //Perform a EFQ to see if the syllabus has a published syllabus
  foreach ($entities as $entity) {
    if (in_array($entity->session_cd, array_keys($sessions))) {
      $three_letter = substr($entity->course_cd, 0, 2);
      $four_number = substr($entity->course_cd, 3, 4);
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'syllabus')
        ->propertyCondition('status', 1)
        ->fieldCondition('field_3_letter_course_code', 'value', $three_letter, '=')
        ->fieldCondition('field_4_number_course_code', 'value', $four_number, '=')
        ->fieldCondition('field_session', 'value', $entity->session_cd, '=')
        ->fieldCondition('field_section', 'value', $entity->section_cd, '=');
      $result = $query->execute();
      //If its empty create a link.
      if (empty($result)) {
        if ($language->language == 'fr') {
          $title = str_replace('-' . $entity->session_cd, ' ' .$sessions[$entity->session_cd]['ses_french_name'], $entity->title); 
        } 
        else {
          $title = str_replace('-' . $entity->session_cd, ' ' .$sessions[$entity->session_cd]['ses_english_name'], $entity->title); 
        }
        $form['pending_list']['#markup'] .= '<li>' . 	l($title, 'syllabus/add', array('query' => array('syllabus' => $entity->id))) . '</li>';
      }
    }
  }

  return $form;
}
