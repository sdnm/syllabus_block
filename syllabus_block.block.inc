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
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'syllabus')
        ->fieldCondition('field_course_code', 'value', $entity->course_cd, '=')
        ->fieldCondition('field_session', 'value', $entity->session_cd, '=')
        ->fieldCondition('field_section', 'value', $entity->section_cd, '=');
      $result = $query->execute();
      //Put this in place to fix a weird issue with node existing but not contatining content.
      $node = '';
      if (isset($result['node'])) {
        $x = reset($result['node']);
        $node = entity_load_single('node', $x->nid);
      }

      if (empty($node)) {
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
  $result = array();
  //Load his groups
  $groups = og_get_groups_by_user();
  $syllabus_groups = entity_load('academic_activity', $groups['academic_activity']);
  //Load his previous nodes
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'syllabus')
        ->propertyCondition('uid', $user->uid);
  $nodes = $query->execute();
  //Also load his group syllabuses. 
  if (isset($nodes['node'])) {
    foreach($nodes['node'] as $node) {
      $result[$node->nid] = $node;
    }
  }

  foreach ($syllabus_groups as $group) {
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
          ->entityCondition('bundle', 'syllabus')
          ->fieldCondition('field_course_code', 'value', $group->course_cd, '=')
          ->fieldCondition('field_session', 'value', $group->session_cd, '=')
          ->fieldCondition('field_section', 'value', $group->section_cd, '=');
    $course_node = $query->execute();
    $node = '';
    if (isset($course_node['node'])) {
      $course_node = reset($course_node['node']);
      $node = entity_load_single('node', $course_node->nid);
    }
    if (!empty($node)) {
      $result[$node->nid] = $node;
    }
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

  foreach ($result as $key => $value) {
    $node = node_load($key);
    if (!empty($node)) {
      global $language;
      $mode = '';
      $title = '';
      $title .= $node->field_course_code[LANGUAGE_NONE][0]['value'];
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
      $status = '';
      if ($node->status == '0') {
        $status = t('Draft');
      }
      elseif ($node->status == '1') {
        $status = t('Published');
      }
      if ($node->field_public[LANGUAGE_NONE][0]['value'] == 1) {
        $status = t('Public');
      }
      $form['syllabus_' . $node->nid] = array(
        '#type' => 'fieldset', 
        '#title' => t(''), 
        '#collapsible' => FALSE, 
        '#collapsed' => FALSE,
      );
      //Link to the node.
      $form['syllabus_' . $node->nid]['link'] = array(
        '#markup' => l($title, 'syllabus/' . $node->nid),
      );
      //Status.
      $form['syllabus_' . $node->nid]['status'] = array(
        '#markup' => '&nbsp;' . $status,
      );
      //Buttons
      if ($node->field_public[LANGUAGE_NONE][0]['value'] == 1) {
        $form['syllabus_' . $node->nid]['public'] = array(
          '#type' => 'submit', 
          '#value' => t('unpublic'),
          '#name' => $node->nid
        );
      }
      else {
        $form['syllabus_' . $node->nid]['public'] = array(
          '#type' => 'submit', 
          '#value' => t('public'),
          '#name' => $node->nid
        );
      }
      if ($node->status == 1) {
        $form['syllabus_' . $node->nid]['publish'] = array(
          '#type' => 'submit', 
          '#value' => t('unpublish'),
          '#name' => $node->nid
        );
      }
      else {
        $form['syllabus_' . $node->nid]['publish'] = array(
          '#type' => 'submit', 
          '#value' => t('publish'),
          '#name' => $node->nid
        );
      }
    }
  }
  return $form;
}

/**
 * Implements hook_form_submit().
 */
function syllabus_mine_form_submit($form, &$form_state) {
  $node = array();
  $matches = array();
  preg_match('/edit-publish/', $form_state['clicked_button']['#id'], $matches);
  if (isset($matches[0]) && $matches[0] == 'edit-publish') {
    $node = node_load($form_state['clicked_button']['#name']);
    if ($node->status) {
      //unpublish, also unset the public flag.
      $node->status = 0;
      $node->field_public[LANGUAGE_NONE][0]['value'] = 0;
    }
    else {
      //Publish.
      $node->status = 1;
    }    
  }
  preg_match('/edit-public/', $form_state['clicked_button']['#id'], $matches);
  if (isset($matches[0]) && $matches[0] == 'edit-public') {
    $node = node_load($form_state['clicked_button']['#name']);
    if ($node->field_public[LANGUAGE_NONE][0]['value']) {
      //unpublic the syllabus
      $node->field_public[LANGUAGE_NONE][0]['value'] = 0;
    }
    else {
      //Publish.
      $node->status = 1;
      $node->field_public[LANGUAGE_NONE][0]['value'] = 1;
    }    
  }
  if (!empty($node)) {
    node_save($node);
  }
}
