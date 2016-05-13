<?php

function worx_commerce_term_pricing_settings_form() {
  $flavor = taxonomy_vocabulary_machine_name_load('product_flavor');
  $fl_terms = entity_load('taxonomy_term', FALSE, array('vid' => $flavor->vid));
  $strength = taxonomy_vocabulary_machine_name_load('product_strength');
  $st_terms = entity_load('taxonomy_term', FALSE, array('vid' => $strength->vid));
  $pgvg = taxonomy_vocabulary_machine_name_load('product_pg_vg');
  $pgvg_terms = entity_load('taxonomy_term', FALSE, array('vid' => $pgvg->vid));
  //dsm($fl_terms);
  //dsm($st_terms);
  //dsm($pgvg_terms);
  $flavor_options = $strength_options = $pgvg_options = array();

  foreach ($fl_terms as $flt_obj) {
    $flavor_options[$fl_terms[$flt_obj->tid]] = $fl_terms[$flt_obj->name];
  }
  dsm($flavor_options);

  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', 'commerce_product')
    ->entityCondition('bundle', 'term_priced')
    ->propertyCondition('status', 1)
    ->fieldOrderBy('commerce_price', 'amount', 'ASC')
  ;
  $result = $query->execute();
  dsm($result);


  $form['contact_information'] = array(
    '#markup' => t('You can leave us a message using the contact form below.'),
  );

  return $form;
}
