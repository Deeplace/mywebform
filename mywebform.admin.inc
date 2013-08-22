<?php

require_once 'mywebform.private.inc';

function mywebform_settings_form($form_state) {
  $form = array();
  $weight = -100;

  $form['mywebform_attachments_amount'] = array(
    '#type' => 'select',
    '#title' => t('Maximal amount of attachments'),
    '#options' => drupal_map_assoc(range(1, 10)),
    '#default_value' => variable_get('mywebform_attachments_amount', 3),
    '#weight' => $weight++,
  );

  $vocabularies = array();
  foreach(taxonomy_get_vocabularies() as $vid => $voc) {
    $vocabularies[$vid] = $voc->name;
  }
  $form['mywebform_vid'] = array(
    '#type' => 'select',
    '#title' => t('Attached vocabulary'),
    '#options' => $vocabularies,
    '#default_value' => variable_get('mywebform_vid', 0),
    '#weight' => $weight++,
  );

  $predefined = mywebform_predefined_values();
  foreach($predefined as $key => $value) {
    $predefined[$key] = $key . ' = ' . $value;
  }

  $form['document_destination_vid'] = array(
    '#type' => 'select',
    '#title' => t('Document destination vocabulary'),
    '#options' => $vocabularies,
    '#default_value' => variable_get('document_destination_vid', 0),
    '#weight' => $weight++,
  );

  $form['predefined_values'] = array(
    '#type' => 'item',
    '#title' => t('Predefined values'),
    '#value' => theme('item_list', $predefined),
    '#weight' => $weight++,
  );

  $form['mywebform_user_defined_values'] = array(
    '#type' => 'textarea',
    '#title' => t('User defined values'),
    '#description' => t('Each pair must be in a separated line. Format: @varname;;value'),
    '#default_value' => variable_get('mywebform_user_defined_values', ''),
    '#weight' => $weight++,
  );

  return system_settings_form($form);
}

function theme_mywebform_manage_form($variables) {
  $form = $variables['form'];
  $output = '';
  $rows = array();

  array_unshift($form['#header'], theme('table_select_header_cell'));
  foreach($form['#rows'] as $key => $row) {
    array_unshift($row, drupal_render($form['checks'][$key]));
    $rows[] = $row;
  }

  if(!empty($form['export'])) {
    $output .= drupal_render($form['export']);
  }
  $output .= drupal_render($form['options']);
  if(empty($rows)) {
    $rows[] = array(
      'data' => array(
        array(
          'data' => t('No results found.'),
          'colspan' => count($form['#header']),
        )
      )
    );
  }
  $output .= theme('table', array('header' => $form['#header'], 'rows' => $rows));

  $output .= drupal_render_children($form);

  return $output;
}

function mywebform_manage_form($form, $form_state) {
  // Execute confirm form for the operation
  if(isset($form_state['values']['operation']) && $form_state['values']['operation'] == 'delete') {
    return mywebform_multiple_delete_confirm($form_state, array_filter($form_state['values']['checks']));
  }

  $form = array();
  $header = array(
    array('data' => t('Name'), 'field' => 'name', 'width' => '50%' ),
    array('data' => t('Status'), 'field' => 'status', ),
    t('Operation'),
    '',
  );
  $rows = array();

  $result = db_select('mywebforms', 'm')
  ->fields('m')
  ->extend('TableSort')
    ->orderByHeader($header)
  ->execute();
  foreach($result as $data) {
    $rows[$data->id] = array(
    l($data->name, 'admin/structure/mywebform/' . $data->id),
    $data->status ? t('Enabled') : t('Disabled'),
    l(t('Export fields'), 'admin/structure/mywebform/' . $data->id . '/export-fields'),
    l(t('Edit'), 'admin/structure/mywebform/' . $data->id . '/edit'),
    );
  }
  $checks = array();
  foreach($rows as $id => $row) {
    $checks[$id] = '';
  }

  $form['#theme'] = 'mywebform_manage_form';
  $form['#header'] = $header;
  $form['#rows'] = $rows;

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => array(
      'disable' => t('Disable'),
      'enable'  => t('Enable'),
      'delete'  => t('Delete'),
    ),
  );

  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  $form['checks'] = array(
    '#type' => 'checkboxes',
    '#options' => $checks,
  );

  return $form;
}

function mywebform_manage_form_validate($form, &$form_state) {
  if($form_state['values']['op'] == t('Update')) {
    $checks = array_filter((array) $form_state['values']['checks']);
    if(!count($checks)) {
      form_set_error('', t('No items selected.'));
    }
  }
}

function mywebform_manage_form_submit($form, &$form_state) {
  if($form_state['values']['op'] == t('Update')) {
    $ids = array_filter($form_state['values']['checks']);

    switch($form_state['values']['operation']) {
      case 'disable':
        db_query("UPDATE {mywebforms} SET status = 0 WHERE id IN (:id)", array(':id' => implode(', ', $ids)));

        break;
      case 'enable':
        db_query("UPDATE {mywebforms} SET status = 1 WHERE id IN (:id) ", array(':id' => implode(', ', $ids)));

        break;
      default:
        $form_state['rebuild'] = TRUE;
        return;
    }
    drupal_set_message(t('The selected items have been updated'));
  }
}

function mywebform_multiple_delete_confirm($form_state, $ids) {
  $form = array();
  $form['#submit'][] = 'mywebform_multiple_delete_confirm_submit';

  $form['checks'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE
  );

  $result = db_query("SELECT id, name FROM {mywebforms} WHERE id IN (:id) ", array(':id' => implode(', ', $ids)));
  foreach($result as $data) {
    $form['checks'][$data->id] = array(
      '#type' => 'hidden',
      '#value' => $data->id,
      '#prefix' => '<li>',
      '#suffix' => check_plain($data->name) ."</li>\n",
    );
  }

  return confirm_form($form, t('Are you sure you want to delete these items?'), $_GET['q']);
}

function mywebform_multiple_delete_confirm_submit($form, &$form_state) {
  if($form_state['values']['confirm']) {
    db_query("DELETE FROM {mywebforms} WHERE id IN (:id) ", array(':id' => implode(', ', $form_state['values']['checks'])));
    drupal_set_message(t('The selected items have been deleted'));
  }
}

function mywebform_manage_fields_form($form, $form_state, $webform = NULL) {
  // Execute confirm form for the operation
  if(isset($form_state['values']['operation']) && $form_state['values']['operation'] == 'delete') {
    return mywebform_fields_multiple_delete_confirm(
      $form_state,
      array_filter($form_state['values']['checks']));
  }

  $form = array();
  $header = array(
    array('data' => t('Name'), 'field' => 'name', ),
    array('data' => t('Title'), 'field' => 'title', ),
    array('data' => t('Type'), 'field' => 'type', ),
    array('data' => t('Widget'), 'field' => 'widget', ),
    array('data' => t('Status'), 'field' => 'status', ),
    t('Operation'),
  );
  $rows = array();

  drupal_set_title(t('Document type <em>@name</em>', array('@name' => $webform->name)),PASS_THROUGH);

  $result = db_select('mywebform_fields', 'mf')
    ->fields('mf')
    ->condition('mywebform_id', $webform->id)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->execute();
  foreach($result as $data) {
    $rows[$data->id] = array(
    l($data->name, 'admin/structure/mywebform/field/' . $data->id . '/view'),
    t($data->title),
    $data->type,
    $data->widget,
    $data->status ? t('Enabled') : t('Disabled'),
    l(t('Edit'), 'admin/structure/mywebform/field/' . $data->id . '/edit'),
    );
  }
  $checks = array();
  foreach($rows as $id => $row) {
    $checks[$id] = '';
  }

  $form['#theme'] = 'mywebform_manage_form';
  $form['#header'] = $header;
  $form['#rows'] = $rows;

  $form['export'] = array(
    '#type' => 'item',
    '#value' => l(t('Export fields'), 'admin/structure/mywebform/' . $webform->id . '/export-fields'),
  );

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#prefix' => '<div class="container-inline">',
    '#suffix' => '</div>',
  );

  $form['options']['operation'] = array(
    '#type' => 'select',
    '#options' => array(
      'disable'    => t('Disable'),
      'enable'     => t('Enable'),
      'delete'     => t('Delete'),
    ),
  );

  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  $form['checks'] = array(
    '#type' => 'checkboxes',
    '#options' => $checks,
  );

  return $form;
}

function mywebform_manage_fields_form_validate($form, &$form_state) {
  if($form_state['values']['op'] == t('Update')) {
    $checks = array_filter((array) $form_state['values']['checks']);
    if(!count($checks)) {
      form_set_error('', t('No items selected.'));
    }
  }
}

function mywebform_manage_fields_form_submit($form, &$form_state) {
  if($form_state['values']['op'] == t('Update')) {
    $ids = array_filter($form_state['values']['checks']);

    switch($form_state['values']['operation']) {
      case 'disable':
        db_query("UPDATE {mywebform_fields} SET status = 0 WHERE id IN (:id) ", array(':id' => implode(', ', $ids)));
        break;
      case 'enable':
        db_query("UPDATE {mywebform_fields} SET status = 1 WHERE id IN (:id) ", array(':id' => implode(', ', $ids)));
        break;
      default:
        $form_state['rebuild'] = TRUE;
        return;
    }
    drupal_set_message(t('The selected items have been updated'));
  }
}

function mywebform_fields_multiple_delete_confirm($form_state, $ids) {
  $form = array();
  $form['#submit'][] = 'mywebform_fields_multiple_delete_confirm_submit';

  $form['checks'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE
  );

  $query = db_select('mywebform_fields', 'm')
    ->fields('m',array('id', 'name'))
    ->condition('m.id', $ids, 'IN');

  $result = $query->execute();
  foreach($result as $data) {
    $form['checks'][$data->id] = array(
      '#type' => 'hidden',
      '#value' => $data->id,
      '#prefix' => '<li>',
      '#suffix' => check_plain($data->name) ."</li>\n",
    );
  }

  return confirm_form($form, t('Are you sure you want to delete these items?'), $_GET['q']);
}

function mywebform_fields_multiple_delete_confirm_submit($form, &$form_state) {
  if($form_state['values']['confirm']) {
    db_query("DELETE FROM {mywebform_fields} WHERE id IN (:id) ", array(':id' => implode(', ', $form_state['values']['checks'])));
    db_delete('mywebform_fields')
      ->condition('id', $form_state['values']['checks'], 'IN')
      ->execute();
    drupal_set_message(t('The selected items have been deleted'));
  }
}

function mywebform_field_page($field = NULL) {
  $output = '';

  if(empty($field)) {
    drupal_not_found();
    exit();
  }

  $result = db_query("SELECT name FROM {mywebforms} WHERE id = :id ", array(':id' => $field->mywebform_id));
  foreach($result as $record) {
    $field->webform = $record->name;
  }

  $names = array(
    'name'          => 'Name',
    'title'         => 'Title',
    'webform'       => 'Document type',
    'weight'        => 'Weight',
    'xpath'         => 'XPath',
    'grid_name'     => 'Grid name',
    'type'          => 'Type',
    'widget'        => 'Widget',
    'disabled'      => 'Disabled',
    'readonly'      => 'Readonly',
    'required'      => 'Required',
    'default_value' => 'Default value',
    'options'       => 'Options',
    'rows'          => 'Rows',
    'check_type'    => 'Check type',
    'class'         => 'Class',
    'minlength'     => 'Minimum length',
    'maxlength'     => 'Maximum length',
    'maxlength_ssemicolon' => 'Maximum length after semicolon',
    'status'        => 'Status',
    'created'       => 'Created',
  );

  $rows = array();
  foreach($names as $key => $title) {
    $rows[] = array('<strong>' . t($title) . '</strong>', htmlspecialchars(isset($field->$key) ? $field->$key : ''));
  }

  $output .= theme('table', array('rows' => $rows));

  return $output;
}

function mywebform_export_fields_page($webform) {
  $header = array();
  $rows = array();

  foreach($webform->fields as $field) {
    unset($field->mywebform_id, $field->status, $field->created, $field->aux);
    if(empty($header)) {
      $header = array_keys((array) $field);
    }
    $rows[] = array_values((array) $field);
  }
  php_excel_export_table($header, $rows, $webform->tech_name . '.xls');
}

function mywebform_type_weight_form($form, $form_state) {
  $result = db_select('mywebforms', 'm')
    ->fields('m')
    ->orderBy('m.weight', 'ASC')
    ->execute();

  $form['#tree'] = TRUE;

  foreach($result as $data) {
    $form[$data->id]['#mywebform'] = $data;
    $form[$data->id]['name'] = array('#markup' => $data->name);
    $form[$data->id]['weight'] = array(
            '#type' => 'weight',
            '#title' => t('Weight for @title', array('@title' => $data->name)),
            '#title_display' => 'invisible',
            '#delta' => 10,
            '#default_value' => $data->weight,
    );
  }

  // Only make this form include a submit button and weight if more than one
  // vocabulary exists.

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save'));

  return $form;
}

function mywebform_type_weight_form_submit($form, &$form_state) {
  foreach ($form_state['values'] as $k => $v) {
    if (is_numeric($k) && $form[$k]['#mywebform']->weight != $form_state['values'][$k]['weight']) {
      $form[$k]['#vocabulary']->weight = $form_state['values'][$k]['weight'];
      db_update('mywebforms')
        ->fields(array('weight' => $form_state['values'][$k]['weight']))
        ->condition('id', $form[$k]['#mywebform']->id)
        ->execute();
    }
  }
  drupal_set_message(t('The configuration options have been saved.'));
}

function theme_mywebform_type_weight_form($variables) {
  $form = $variables['form'];

  $rows = array();

  foreach (element_children($form) as $key) {
    if (isset($form[$key]['name'])) {
      $mywebform = &$form[$key];

      $row = array();
      $row[] = drupal_render($mywebform['name']);
      if (isset($mywebform['weight'])) {
        $mywebform['weight']['#attributes']['class'] = array('mywebform-weight');
        $row[] = drupal_render($mywebform['weight']);
      }
      $rows[] = array('data' => $row, 'class' => array('draggable'));
    }
  }

  $header = array(t('Name'));
  if (isset($form['actions'])) {
    $header[] = t('Weight');
    drupal_add_tabledrag('mywebform', 'order', 'sibling', 'mywebform-weight');
  }
  //$header[] = array('data' => t('Operations'), 'colspan' => '3');
  $header = array();
  return theme('table', array('header' => $header, 'rows' => $rows, 'empty' => t('No mywebform available.'), 'attributes' => array('id' => 'mywebform'))) . drupal_render_children($form);
}