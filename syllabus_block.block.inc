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
    '#markup' => t('Here is a list of the pending syllabi.'),
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
      $three_letter = substr($entity->course_cd, 0, 3);
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
      if (empty($result)) {
        $response = restclient_get('session/' . $entity->session_cd);
        $xml = _syllabus_api_parse_xml($response->data);
        if ($language->language == 'fr') {
          $title = str_replace('-' . $entity->session_cd, ' ' .$xml['entityResult']['ses_french_name'], $entity->title); 
        } 
        else {
          $title = str_replace('-' . $entity->session_cd, ' ' .$xml['entityResult']['ses_english_name'], $entity->title); 
        }
        $form['pending_list']['#markup'] .= '<li>' . 	l($title, 'syllabus/add', array('query' => array('syllabus' => $entity->id))) . '</li>';
      }
    }
  }
  return $form;
}


/**
 * Implement hook_form().
 */
function syllabus_mine_form($form, &$form_state) {
  global $user;
  //Load his groups
  $groups = og_get_groups_by_user();
  $entities = entity_load('academic_activity', $groups['academic_activity']);
  //Load his previous nodes
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'syllabus')
        ->propertyCondition('uid', $user->uid);
  $nodes = $query->execute();
  $nodes = $nodes['node'];

  foreach ($entities as $entity) {
    $three_letter = substr($entity->course_cd, 0, 3);
    $four_number = substr($entity->course_cd, 3, 4);
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
          ->entityCondition('bundle', 'syllabus')
          ->fieldCondition('field_3_letter_course_code', 'value', $three_letter, '=')
          ->fieldCondition('field_4_number_course_code', 'value', $four_number, '=')
          ->fieldCondition('field_session', 'value', $entity->session_cd, '=')
          ->fieldCondition('field_section', 'value', $entity->section_cd, '=');
    $result = $query->execute();
    //If its empty create a link.
    if (!empty($result)) {
      $nodes = array_merge($nodes, $result['node']); 
    }
  }
  $result = array();
  foreach($nodes as $node) {
    $result[$node->nid] = $node;
  }

  $form['legend_draft'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#markup' => t('Draft, not published still in the process of being created.'),
  );
  $form['legend_published'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#markup' => t('Published, Syllabus has been created and can be shared with student and professors.'),
  );
  $form['legend_public'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#markup' => t('Public, Syllabus is publically searchable.'),
  );

  $form['pending_list'] = array(
    '#prefix' => '<ul>',
    '#markup' => '',
    '#suffix' => '</ul>',
  );
  foreach ($result as $key => $value) {
    $node = node_load($key);
    if (!empty($node)) {
      global $language;
      $mode = '';
      $title = '';
      $title .= $node->field_3_letter_course_code[LANGUAGE_NONE][0]['value'];
      $title .= $node->field_4_number_course_code[LANGUAGE_NONE][0]['value'];
      $title .= '-';
      $title .= $node->field_section[LANGUAGE_NONE][0]['value'];
      $title .= ' ';

      $response = restclient_get('session/' . $node->field_session[LANGUAGE_NONE][0]['value']);
      $xml = _syllabus_api_parse_xml($response->data);
      if ($language->language == 'fr') {
        if (isset($xml['entityResult']['ses_french_name'])) {
          $title .= $xml['entityResult']['ses_french_name'];
        }
        else {
          $title .= $node->field_session[LANGUAGE_NONE][0]['value'];
        }
      }
      else {
        if (isset($xml['entityResult']['ses_english_name'])) {
          $title .= $xml['entityResult']['ses_english_name'];
        }
        else {
          $title .= $node->field_session[LANGUAGE_NONE][0]['value'];
        }
      }
      $title .= ' ';
      if ($node->status == '0') {
        $title .= t('(Draft)');
      }
      elseif($node->status == '1') {
        $title .= t('(Published)');
      }
      elseif(!empty($node->field_public)) {
        $title .= t('(Public)');
      }
      $form['pending_list']['#markup'] .= '<li>' ;
      $form['pending_list']['#markup'] .= l($title, 'syllabus/' . $node->nid);
      $form['pending_list']['#markup'] .= '</li>';
    }
  }
  return $form;
}
